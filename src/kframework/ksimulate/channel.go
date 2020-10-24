package ksimulate

import (
	"io/ioutil"
	"os"
	"strings"

	"github.com/lishaoliang/klb/src/kfile"
	"github.com/lishaoliang/klb/src/knet/kflv/kflvreader"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

type channel struct {
	path string
	id   uint32

	timeOpen int64
	files    []string

	reader kfile.Reader

	baseTime      int64
	baseFrameTime int64

	frame     kmnp.MnpMedia
	frameData []byte
}

func openChnn(id uint32) *channel {
	var m channel

	m.id = id

	return &m
}

func (m *channel) SetPath(path string) {
	m.path = path
}

func (m *channel) getNextFile() string {
	var files []string

	if nil == m.files {
		m.files, _ = getAllFiles(m.path)
		//m.files = append(files, "aaa.flv")
	}

	if 0 < len(m.files) {
		path := m.files[0]
		m.files = append(files, m.files[1:]...)
		//fmt.Println("files:", m.files)

		return path
	}

	return ""
}

func (m *channel) tryOpen(now int64) bool {

	if kutil.SubAbsI64(now, m.timeOpen) < int64(1000000)*180 {
		return false
	}

	if nil != m.reader {
		m.reader.Close()
		m.reader = nil
	}

	path := m.getNextFile()
	if "" != path {
		m.reader, _ = kflvreader.Open(path)
		//fmt.Println("Open", path)
	}

	if nil != m.reader {
		m.timeOpen = now - int64(1000000)*180
	} else {
		m.timeOpen = now
	}

	m.baseTime = 0
	m.baseFrameTime = 0

	m.frameData = nil

	return (nil != m.reader)
}

func (m *channel) Close() {
	if nil != m.reader {
		m.reader.Close()
		m.reader = nil
	}
}

func (m *channel) OnTimer(now int64) {
	for {
		if nil == m.reader {
			if !m.tryOpen(now) {
				break
			}
		}

		if nil == m.frameData {
			var err error

			m.frame, m.frameData, err = m.reader.ReadNext()
			if nil != err {
				m.reader.Close()
				m.reader = nil
				continue
			}
		}

		if nil != m.frameData {
			if kmnp.MnpDtypeAAC == m.frame.Dtype {
				md := m.frame
				md.Chnn = m.id
				md.Sidx = kmnp.MnpSidxA1

				pushStream(md, m.frameData)
				m.frameData = nil
				continue
			}

			bPush := false

			if 0 == m.baseTime {
				bPush = true
				m.baseTime = now
				m.baseFrameTime = m.frame.Time
			} else {
				dtTime := kutil.SubAbsI64(now, m.baseTime)
				dtFrameTime := kutil.SubAbsI64(m.frame.Time, m.baseFrameTime)

				if dtFrameTime <= dtTime+6000 {
					bPush = true
				}
			}

			if bPush {
				md := m.frame
				md.Chnn = m.id
				md.Sidx = kmnp.MnpSidxV1

				pushStream(md, m.frameData)
				m.frameData = nil
			}
		}

		break
	}
}

func getAllFiles(dirPath string) (files []string, err error) {
	var dirs []string
	dir, err := ioutil.ReadDir(dirPath)
	if err != nil {
		return nil, err
	}

	pathSep := string(os.PathSeparator)

	for _, fi := range dir {
		if fi.IsDir() { // 目录, 递归遍历
			dirs = append(dirs, dirPath+pathSep+fi.Name())
			getAllFiles(dirPath + pathSep + fi.Name())
		} else {
			// 过滤指定格式
			ok := strings.HasSuffix(fi.Name(), ".flv")
			if ok {
				files = append(files, dirPath+pathSep+fi.Name())
			}
		}
	}

	// 读取子目录下文件
	for _, table := range dirs {
		temp, _ := getAllFiles(table)
		for _, temp1 := range temp {
			files = append(files, temp1)
		}
	}

	return files, nil
}
