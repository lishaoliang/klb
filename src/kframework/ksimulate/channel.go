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
	last   int64
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

		var md kmnp.MnpMedia
		var d []byte
		var err error

		md, d, err = m.reader.ReadNext()
		if nil != err {
			m.reader.Close()
			m.reader = nil
			continue
		}

		if kmnp.MnpDtypeNil != md.Dtype {
			kutil.Assert(0 < len(d))

			md.Chnn = m.id
			md.Sidx = kmnp.MnpSidxV1
			md.Time = now

			pushStream(md, d)
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
