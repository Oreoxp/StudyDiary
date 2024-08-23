package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"golang.org/x/net/http2"
	"io"
	"net"
)

type FrameType uint8

type Flags uint8

type FrameHeader struct {
	valid bool // caller can access []byte fields in the Frame

	// Type is the 1 byte frame type. There are ten standard frame
	// types, but extension frame types may be written by WriteRawFrame
	// and will be returned by ReadFrame (as UnknownFrame).
	Type FrameType

	// Flags are the 1 byte of 8 potential bit flags per frame.
	// They are specific to the frame type.
	Flags Flags

	// Length is the length of the frame, not including the 9 byte header.
	// The maximum size is one byte less than 16MB (uint24), but only
	// frames up to 16KB are allowed without peer agreement.
	Length uint32

	// StreamID is which stream this frame is for. Certain frames
	// are not stream-specific, in which case this field is 0.
	StreamID uint32
}

type WindowUpdateFrame struct {
	FrameHeader
	Increment uint32 // never read with high bit set
}

func handleWindowUpdate(f *http2.WindowUpdateFrame) {
	streamID := f.Header().StreamID
	increment := f.Increment

	fmt.Println(streamID, increment)
}

type FrameHeader2 struct {
	Length   uint32
	Type     uint8
	Flags    uint8
	StreamID uint32
}

func parseFrameHeader(buf []byte) error {
	var header FrameHeader2

	// Check if the buffer has at least 9 bytes (frame header size)
	if len(buf) < 9 {
		return fmt.Errorf("Insufficient data to parse frame header")
	}

	for i := 0; i < NumSettings(buf); {
		header.Length = (uint32(buf[i])<<16 | uint32(buf[i+1])<<8 | uint32(buf[i+2]))
		header.Type = buf[i+3]
		header.Flags = buf[i+4]
		header.StreamID = binary.BigEndian.Uint32(buf[5:]) & (1<<31 - 1) // Remove the highest bit
		i += int(header.Length) + 9
		fmt.Printf("Frame Length: %d", header.Length)
		fmt.Printf(" Frame Type: %d", header.Type)
		fmt.Printf(" Frame Flags: %d", header.Flags)
		fmt.Printf(" Stream ID: %d", header.StreamID)
		fmt.Printf("\n")
	}

	return nil
}

func NumSettings(payload []byte) int { return len(payload) / 6 }

type SettingID uint16

func parseSettingsFramePayload(payload []byte) {
	for i := 0; i < NumSettings(payload); i++ { // At least 6 bytes (2 bytes for ID + 4 bytes for value)
		paramID := SettingID(binary.BigEndian.Uint16(payload[i*6 : i*6+2]))
		paramValue := binary.BigEndian.Uint32(payload[i*6+2 : i*6+6])

		switch paramID {
		case 0x3:
			fmt.Printf("Setting ID 11: %d, Value: %d\n", paramID, paramValue)
		case 0x6:
			fmt.Printf("Setting ID 22: %d, Value: %d\n", paramID, paramValue)
		}
	}
}

func http2Parse(payload []byte) {
	reader := bytes.NewReader(payload)
	framer := http2.NewFramer(nil, reader)

	for {
		frame, err := framer.ReadFrame()
		if err == io.EOF {
			break // 结束循环
		} else if err != nil {
			fmt.Println("Error reading frame:", err)
			continue
		}

		fmt.Println("http2 Frame Type:", frame.Header().Type)
		fmt.Println("http2 Frame Length:", frame.Header().Length)
		fmt.Println("http2 Frame Flags:", frame.Header().Flags)
		fmt.Println("http2 Frame Stream ID:", frame.Header().StreamID)

		// 在这里可以根据帧类型进一步解析 framePayload
	}
}

func main() {
	serverAddr := "127.0.0.1:50051" // 修改为你的服务器地址

	// 创建与服务端的连接
	conn, err := net.Dial("tcp", serverAddr)
	if err != nil {
		fmt.Println("Error connecting:", err)
		return
	}
	/*
		// 等待服务端发送帧设置数据
		read := make([]byte, 1540)
		_, err = conn.Read(read)
		if err != nil {
			fmt.Println("Error read preface:", err)
			return
		}

		err = parseFrameHeader(read)
		if err != nil {
			fmt.Println("Error parsing frame header:", err)
			return
		}

		parseSettingsFramePayload(read)

		fmt.Println("Preface sent successfully!  ", read)
		http2Parse(read)*/

	for {
		readbuf := make([]byte, 1024)
		n, err := conn.Read(readbuf)
		if n > 0 {
			// 处理读取到的数据，例如打印或存储
			fmt.Print(readbuf[:n])
			http2Parse(readbuf)
		}
		if err == io.EOF {
			// 读取完成
			break
		} else if err != nil {
			fmt.Println("Error reading response body:", err)
			break
		}
	}

	defer conn.Close()

	// HTTP/2 连接预热字符串
	preface := "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

	// 向服务端发送连接预热字符串
	_, err = conn.Write([]byte(preface))
	if err != nil {
		fmt.Println("Error sending preface:", err)
		return
	}

	//流处理阶段

}
