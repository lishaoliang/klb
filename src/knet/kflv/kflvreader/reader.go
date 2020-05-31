package kflvreader

import (
	"bytes"
	"encoding/binary"
	"errors"
	"os"

	"github.com/lishaoliang/klb/src/knet/kflv"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil/kh26x"
)

// reader 按音视频帧读取
// kfile.Reader
type reader struct {
	file   *os.File // 系统文件句柄
	offset int64    // 当前的偏移

	head     kflv.Head  // 读取的flv文件头
	tag      kflv.Tag   // Tag
	tagVideo kflv.Video // Tag video

	avc     kflv.Avc          // AVC
	avcCfg  kflv.AvcDecCfgRec // AVC编码配置信息
	sps     []byte            // AVC/SPS
	pps     []byte            // AVC/PPS
	avcNalu [][]byte          // AVC的NALU单元
}

func (m *reader) Close() error {

	m.Reset()

	if nil != m.file {
		return m.file.Close()
	}

	return nil
}

var h26xBegin = []byte{0x0, 0x0, 0x0, 0x1}

func (m *reader) Reset() error {
	m.offset = int64(m.head.Offset) + 4

	m.sps = nil
	m.pps = nil
	m.avcNalu = nil

	return nil
}

func (m *reader) ReadNext() (kmnp.MnpMedia, []byte, error) {
	var err error
	var md kmnp.MnpMedia
	var data []byte

	bKey := false

	for {
		err = m.readNextTag()
		if nil != err {
			break
		}

		if nil != m.avcNalu {
			var frame [][]byte

			for i := 0; i < len(m.avcNalu); i++ {
				if kh26x.H264IsI(m.avcNalu[i][0]) {
					bKey = true
				}
			}

			if bKey {
				frame = append(frame, h26xBegin, m.sps, h26xBegin, m.pps)
			}

			for i := 0; i < len(m.avcNalu); i++ {
				frame = append(frame, h26xBegin, m.avcNalu[i])
			}

			data = bytes.Join(frame, nil)
			//fmt.Println("frame", d[0:kutil.Min(128, len(data))])

			m.avcNalu = nil
			break
		}
	}

	if nil != data {
		md.Size = uint32(len(data) + kmnp.MnpMediaSize)
		md.Dtype = kmnp.MnpDtypeH264
		md.Chnn = 0
		md.Sidx = kmnp.MnpSidxV1
		md.Time = int64(m.tag.Timestamp) * 1000

		if bKey {
			md.Vtype = kmnp.MnpVtypeI
		} else {
			md.Vtype = kmnp.MnpVtypeP
		}
	}

	//fmt.Println("time:", m.tag.TimestampEx, m.tag.Timestamp, m.avc.CompositionTime)

	return md, data, err
}

func (m *reader) open(name string) error {
	f, err := os.Open(name)
	//fmt.Println("", f, err)

	if nil != err {
		return err
	}

	b := make([]byte, kflv.FlvHeadSize)
	n, _ := f.Read(b)

	if kflv.FlvHeadSize != n {
		goto errOpen
	}

	m.head.Unpack(b)
	if !m.head.IsOk() {
		goto errOpen
	}

	// 第一个tag位置, 额外偏移4字节
	// 因为组织方式为: 上一个Tag的大小(4字节) + Tag数据
	m.offset = int64(m.head.Offset) + 4

	m.file = f
	return nil

errOpen:
	f.Close()
	return errors.New("kflv.kflvreader.reader.open file error!data err")
}

func (m *reader) readNextTag() error {
	m.file.Seek(m.offset, os.SEEK_SET)

	t := make([]byte, kflv.FlvTagSize)
	_, err := m.file.Read(t)

	if nil != err {
		return err
	}

	var tag kflv.Tag
	tag.Unpack(t)
	m.tag = tag

	b := make([]byte, tag.DataSize)
	_, err = m.file.Read(b)

	if nil != err {
		return err
	}

	switch tag.TagType {
	case kflv.FlvTagScript:
		// 略过
	case kflv.FlvTagAudio:
		m.parserTagAudio(b)
	case kflv.FlvTagVideo:
		m.parserTagVideo(b)
	}

	size := int64(tag.DataSize + kflv.FlvTagSize)
	m.offset += size + 4

	return nil
}

func (m *reader) parserTagScript(b []byte) {

}

func (m *reader) parserTagAudio(b []byte) {

}

func (m *reader) parserTagVideo(b []byte) {
	var video kflv.Video
	video.Unpack(b[0:1])
	b = b[1:]
	m.tagVideo = video

	//fmt.Println("video", video)

	if kflv.FlvCodecAvc == video.CodecID {
		m.parserTagVideoAVC(b)
	}
}

func (m *reader) parserTagVideoAVC(b []byte) {
	//fmt.Println("parserTagVideoAVC", b[0:kutil.Min(64, len(b))])

	// AVC
	var avc kflv.Avc
	avc.Unpack(b[0:kflv.FlvAvcSize])
	b = b[kflv.FlvAvcSize:]
	m.avc = avc

	//fmt.Println("avc", avc)

	if kflv.FlvAvcSequence == avc.AvcPackType {
		m.parserTagVideoAvcSequence(b) // sps, pps
	} else if kflv.FlvAvcNalu == avc.AvcPackType {
		m.parserTagVideoAvcNalu(b) // I,P帧;
	} else {
		// 忽略: kflv.FLVavcEnd
	}
}

func (m *reader) parserTagVideoAvcSequence(b []byte) {
	// sps,pps
	var cfg kflv.AvcDecCfgRec
	cfg.Unpack(b[0:kflv.FlvAvcDecCfgRecSize])
	m.avcCfg = cfg

	//fmt.Println("parserTagVideoAvcSequence", b)
	//fmt.Println("cfg", cfg)

	b = b[kflv.FlvAvcDecCfgRecSize:]

	numSpsCount := b[0]
	b = b[1:]
	spsCount := numSpsCount & 0x1F
	first := true

	for i := byte(0); i < spsCount; i++ {
		spsLen := binary.BigEndian.Uint16(b[0:2])

		sps := b[2 : spsLen+2]
		//fmt.Println("SPS", spsLen, sps)

		if first && 0 < spsLen {
			m.sps = sps // 取此数据中的第一个sps, 且sps信息可以被后续数据更新
			first = false
		}

		b = b[spsLen+2:]
	}

	ppsCount := b[0]
	b = b[1:]
	first = true

	for i := byte(0); i < ppsCount; i++ {
		ppsLen := binary.BigEndian.Uint16(b[0:2])

		pps := b[2 : ppsLen+2]
		//fmt.Println("PPS", ppsLen, pps)

		if first && 0 < ppsLen {
			m.pps = pps // 取此数据中的第一个pps, 且pps信息可以被后续数据更新
			first = false
		}

		b = b[ppsLen+2:]
	}
}

func (m *reader) parserTagVideoAvcNalu(b []byte) {
	// I,P帧
	var datas [][]byte

	for 0 < len(b) {
		naluSize := binary.BigEndian.Uint32(b[0:4])

		if 3 == m.avcCfg.SizeMinusOne {
			naluSize = naluSize >> 8

			nalu := b[3 : naluSize+3]
			//fmt.Println("nalu3", naluSize, nalu[0:kutil.Min(16, len(nalu))])
			datas = append(datas, nalu)

			b = b[naluSize+3:]
		} else {
			nalu := b[4 : naluSize+4]
			//fmt.Println("nalu4", naluSize, nalu[0:kutil.Min(16, len(nalu))])
			datas = append(datas, nalu)

			b = b[naluSize+4:]
		}
	}

	m.avcNalu = datas
}
