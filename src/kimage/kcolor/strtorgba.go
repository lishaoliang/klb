///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   strtorgba.go
/// @author lishaoliang
/// @brief	color string to rgba
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"errors"
	"image/color"
	"regexp"
	"strconv"
)

// StrToRGBA string to RGBA
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrToRGBA(c string) color.RGBA {
	if rgba, err := strToRGBAFmt1(c); nil == err {
		return rgba
	}

	if rgba, err := strToRGBAFmt2(c); nil == err {
		return rgba
	}

	if rgba, err := strToRGBAFmt3(c); nil == err {
		return rgba
	}

	return color.RGBA{}
}

func strToRGBAFmt1(c string) (color.RGBA, error) {
	// "#3FE03BFF"
	// "#3FE03B"
	reg := regexp.MustCompile(`#[0-9a-fA-F]{0,8}`)
	s := reg.FindString(c)

	if 0 < len(s) {
		reg1 := regexp.MustCompile(`[0-9a-fA-F]{0,2}`)
		v := reg1.FindAllString(s[1:], 4)

		var r [4]int64
		r[3] = 255

		for i := 0; i < len(v); i++ {
			ss := v[i]
			if len(ss) < 2 {
				ss += "0"
			}

			r[i], _ = strconv.ParseInt(ss, 16, 0)
		}

		return color.RGBA{uint8(r[0]), uint8(r[1]), uint8(r[2]), uint8(r[3])}, nil
	}

	return color.RGBA{}, errors.New("kimage.kcolor.strtorgba.strToRGBAFmt1 error!FindString")
}

func strToRGBAFmt2(c string) (color.RGBA, error) {
	// "rgba(0, 0, 0, 255)"
	reg := regexp.MustCompile(`rgba\(([\d]*),([\d]*),([\d]*),([\d]*)\)`)
	s := reg.FindStringSubmatch(c)

	if 0 < len(s) {
		r, _ := strconv.ParseInt(s[1], 10, 0)
		g, _ := strconv.ParseInt(s[2], 10, 0)
		b, _ := strconv.ParseInt(s[3], 10, 0)
		a, _ := strconv.ParseInt(s[4], 10, 0)

		return color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)}, nil
	}

	return color.RGBA{}, errors.New("kimage.kcolor.strtorgba.strToRGBAFmt2 error!FindString")
}

func strToRGBAFmt3(c string) (color.RGBA, error) {
	// "rgb(0, 0, 0)"
	reg := regexp.MustCompile(`rgb\(([\d]*),([\d]*),([\d]*)\)`)
	s := reg.FindStringSubmatch(c)

	if 0 < len(s) {
		r, _ := strconv.ParseInt(s[1], 10, 0)
		g, _ := strconv.ParseInt(s[2], 10, 0)
		b, _ := strconv.ParseInt(s[3], 10, 0)

		return color.RGBA{uint8(r), uint8(g), uint8(b), 255}, nil
	}

	return color.RGBA{}, errors.New("kimage.kcolor.strtorgba.strToRGBAFmt3 error!FindString")
}
