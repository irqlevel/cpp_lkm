package main

import (
    "bytes"
    "encoding/binary"
    "errors"
    "fmt"
    "io"
    "log"
    "net"
    "os"
    "github.com/pborman/uuid"
)

const (
    PacketMaxDataSize = 2 * 65536
    PacketMagic       = 0xCCBECCBE
    PacketTypePing    = 1
    PacketTypeChunkWrite = 2
    PacketTypeChunkRead = 3
    PacketTypeChunkDelete = 4
    ChunkSize = 65536
    GuidSize = 16
)

type Client struct {
    Host string
    Con  net.Conn
}

type PacketHeader struct {
    Magic    uint32
    Type     uint32
    DataSize uint32
    Result   uint32
}

type Packet struct {
	Header PacketHeader
	Body   []byte
}

type ToBytes interface {
	ToBytes() ([]byte, error)
}

type ParseBytes interface {
	ParseBytes(body []byte) error
}

type ReqPing struct {
	Value [PacketMaxDataSize]byte
}

type RespPing struct {
	Value [PacketMaxDataSize]byte
}

type ReqChunkWrite struct {
    ChunkId [GuidSize]byte
    Data [ChunkSize]byte
}

type RespChunkWrite struct {
}

type ReqChunkRead struct {
    ChunkId [GuidSize]byte
}

type RespChunkRead struct {
    Data [ChunkSize]byte
}

type ReqChunkDelete struct {
    ChunkId [GuidSize]byte
}

type RespChunkDelete struct {
}

func (req *ReqPing) ToBytes() ([]byte, error) {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, req)
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (resp *RespPing) ParseBytes(body []byte) error {
	err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
	if err != nil {
		return err
	}
	return nil
}

func (req *ReqChunkWrite) ToBytes() ([]byte, error) {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, req)
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (resp *RespChunkWrite) ParseBytes(body []byte) error {
	err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
	if err != nil {
		return err
	}
	return nil
}

func (req *ReqChunkRead) ToBytes() ([]byte, error) {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, req)
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (resp *RespChunkRead) ParseBytes(body []byte) error {
	err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
	if err != nil {
		return err
	}
	return nil
}

func (req *ReqChunkDelete) ToBytes() ([]byte, error) {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, req)
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (resp *RespChunkDelete) ParseBytes(body []byte) error {
	err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
	if err != nil {
		return err
	}
	return nil
}

func NewClient(host string) *Client {
	client := new(Client)
	client.Host = host
	return client
}

func (client *Client) Dial() error {
	con, err := net.Dial("tcp", client.Host)
	if err != nil {
		return err
	}
	client.Con = con
	return nil
}

func (client *Client) CreatePacket(packetType uint32, body []byte) *Packet {
	packet := new(Packet)
	packet.Header.Magic = PacketMagic
	packet.Header.Type = packetType
	packet.Header.DataSize = uint32(len(body))
	packet.Header.Result = 0
	packet.Body = body
	return packet
}

func (client *Client) SendPacket(packet *Packet) error {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.LittleEndian, &packet.Header)
	if err != nil {
		return err
	}

	err = binary.Write(buf, binary.LittleEndian, packet.Body)
	if err != nil {
		return err
	}

	n, err := client.Con.Write(buf.Bytes())
	if err != nil {
		return err
	}

	if n != buf.Len() {
		return errors.New("Incomplete I/O")
	}

	return nil
}

func (client *Client) RecvPacket() (*Packet, error) {
        log.Printf("Receiving packet header\n")
	packet := new(Packet)
	err := binary.Read(client.Con, binary.LittleEndian, &packet.Header)
	if err != nil {
		return nil, err
	}

	if packet.Header.Magic != PacketMagic {
		return nil, errors.New("Invalid packet magic")
	}

	if packet.Header.DataSize > PacketMaxDataSize {
		return nil, errors.New("Packet data size too big")
	}
        log.Printf("Receiving packet body\n")
	body := make([]byte, packet.Header.DataSize)
	if packet.Header.DataSize != 0 {
		n, err := io.ReadFull(client.Con, body)
		if err != nil {
			return nil, err
		}

		if uint32(n) != packet.Header.DataSize {
			return nil, errors.New("Incomplete I/O")
		}
	}
	packet.Body = body

	return packet, nil
}

func (client *Client) MakePacket(reqType uint32, req ToBytes) (*Packet, error) {
	body, err := req.ToBytes()
	if err != nil {
		return nil, err
	}
	return client.CreatePacket(reqType, body), nil
}

func (client *Client) SendRequest(reqType uint32, req ToBytes) error {
	packet, err := client.MakePacket(reqType, req)
	if err != nil {
		return err
	}

	return client.SendPacket(packet)
}

func (client *Client) RecvResponse(respType uint32, resp ParseBytes) error {
	packet, err := client.RecvPacket()
	if err != nil {
		return err
	}

	if packet.Header.Type != respType {
		return fmt.Errorf("Unexpected packet type %d, should be %d",
			packet.Header.Type, respType)
	}

	if packet.Header.Result != 0 {
		return fmt.Errorf("Packet error: %d", int32(packet.Header.Result))
	}

	return resp.ParseBytes(packet.Body)
}

func (client *Client) SendRecv(reqType uint32, req ToBytes, resp ParseBytes) error {
    log.Printf("Sending request\n")
    err := client.SendRequest(reqType, req)
    if err != nil {
        return err
    }
    log.Printf("Receiving response\n")
    err = client.RecvResponse(reqType, resp)
    if err != nil {
        return err
    }

    return nil
}

func getString(bytes []byte) string {
    for i, _ := range bytes {
        if bytes[i] == 0 {
            return string(bytes[:i])
        }
    }

    return string(bytes[:len(bytes)])
}

func (client *Client) Ping(value string) (string, error) {
    req := new(ReqPing)
    valueBytes := []byte(value)
    if len(valueBytes) > len(req.Value) {
        return "", errors.New("Key too big")
    }
    copy(req.Value[:len(req.Value)], valueBytes)

    resp := new(RespPing)
    err := client.SendRecv(PacketTypePing, req, resp)
    if err != nil {
        return "", err
    }

    return getString(resp.Value[:len(resp.Value)]), nil
}

func (client *Client) ChunkWrite(chunkId []byte, data []byte) (error) {
    req := new(ReqChunkWrite)
    if len(chunkId) != len(req.ChunkId) {
        return errors.New("Invalid chunk id size")
    }
    if len(data) != len(req.Data) {
        return errors.New("Invalid data size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])
    copy(req.Data[:len(req.Data)], data[:len(req.Data)])

    resp := new(RespChunkWrite)
    err := client.SendRecv(PacketTypeChunkWrite, req, resp)
    if err != nil {
        return err
    }

    return nil
}

func (client *Client) ChunkRead(chunkId []byte) ([]byte, error) {
    req := new(ReqChunkRead)
    if len(chunkId) != len(req.ChunkId) {
        return nil, errors.New("Invalid chunk id size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])

    resp := new(RespChunkRead)
    err := client.SendRecv(PacketTypeChunkRead, req, resp)
    if err != nil {
        return nil, err
    }

    return resp.Data[:len(resp.Data)], nil
}

func (client *Client) ChunkDelete(chunkId []byte) error {
    req := new(ReqChunkDelete)
    if len(chunkId) != len(req.ChunkId) {
        return errors.New("Invalid chunk id size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])

    resp := new(RespChunkDelete)
    err := client.SendRecv(PacketTypeChunkDelete, req, resp)
    if err != nil {
        return err
    }

    return nil
}

func (client *Client) Close() {
    if client.Con != nil {
        client.Con.Close()
    }
}

func main() {
    log.SetFlags(0)
    log.SetOutput(os.Stdout)

    client := NewClient("127.0.0.1:8111")
    err := client.Dial()
    if err != nil {
        log.Printf("Dial failed: %v\n", err)
	os.Exit(1)
	return
    }
    defer client.Close()

    result, err := client.Ping("Hello world!")
    if err != nil {
        log.Printf("Ping failed: %v\n", err)
        os.Exit(1)
        return
    }
    log.Printf("Ping result %s\n", result)

    chunkId := uuid.NewRandom()[:]
    err = client.ChunkWrite(chunkId, make([]byte, ChunkSize, ChunkSize))
    if err != nil {
        log.Printf("Chunk write failed: %v\n", err)
        os.Exit(1)
        return
    }

    _, err = client.ChunkRead(chunkId)
    if err != nil {
        log.Printf("Chunk read failed: %v\n", err)
        os.Exit(1)
        return
    }

    err = client.ChunkDelete(chunkId)
    if err != nil {
        log.Printf("Chunk delete failed: %v\n", err)
        os.Exit(1)
        return
    }
}
