#include "bio.h"
#include "trace.h"

Bio::Bio(int pageCount, Error& err)
    : BioPtr(nullptr)
    , PageCount(pageCount)
    , IoError(Error::NotExecuted)
{
    if (err != Error::Success)
    {
        return;
    }

    if (pageCount == 0)
    {
        err = Error::InvalidValue;
        return;
    }

    BioPtr = get_kapi()->alloc_bio(pageCount);
    if (!BioPtr)
    {
        trace(0, "Can't allocate bio");
        err = Error::NoMemory;
        return;
    }

    int rc = get_kapi()->set_bio_end_io(BioPtr, &Bio::EndIo, this);
    if (rc)
    {
        trace(0, "Can't set bio private");
        get_kapi()->free_bio(BioPtr);
        BioPtr = nullptr;
        err = Error(rc);
        return;
    }

    trace(1, "Bio 0x%p bio 0x%p contructed\n", this, BioPtr);
}

void Bio::SetBdev(BlockDevice& blockDevice)
{
    get_kapi()->set_bio_bdev(BioPtr, blockDevice.GetBdev());
}

void Bio::SetRead()
{
    get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_READ);
}

void Bio::SetWrite()
{
    get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_WRITE);
}

void Bio::SetFua()
{
    get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_FUA);
}

void Bio::SetFlush()
{
    get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_FLUSH);
}

Error Bio::SetPage(int pageIndex, Page& page, int offset, int len)
{
    int rc = get_kapi()->set_bio_page(BioPtr, pageIndex, page.GetPage(), offset, len);
    if (rc)
    {
        trace(0, "Can't set bio page, rc %d", rc);
    }
    return Error(rc);
}

void Bio::EndIo(int err)
{
    trace(1, "Bio 0x%p bio 0x%p endio err %d", this, BioPtr, err);
    IoError.SetCode(err);
    EndIoEvent.Set();
}

void Bio::Wait()
{
    EndIoEvent.Wait();
}

void Bio::EndIo(void* bio, int err)
{
    Bio* bio_ = static_cast<Bio*>(get_kapi()->get_bio_private(bio));
    bio_->EndIo(err);
}

void Bio::SetPosition(unsigned long long sector)
{
    get_kapi()->set_bio_position(BioPtr, sector);
}

void Bio::Submit()
{
    trace(1, "Bio 0x%p bio 0x%p submit", this, BioPtr);
    get_kapi()->submit_bio(BioPtr);
}

Error Bio::GetError()
{
    return IoError;
}

Bio::~Bio()
{
    trace(1, "Bio 0x%p bio 0x%p dectructor", this, BioPtr);
    if (BioPtr != nullptr)
    {
        get_kapi()->free_bio(BioPtr);
        BioPtr = nullptr;
    }
}