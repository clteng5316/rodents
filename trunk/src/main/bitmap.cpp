// bitmap.cpp

#include "stdafx.h"
#include "win32.hpp"
#include "math.hpp"

static Color SysColor(int n)
{
	COLORREF c = ::GetSysColor(n);
	return Color(GetRValue(c), GetGValue(c), GetBValue(c));
}

static void CopyBits(void* dst, int dststride, const void* src, int srcstride, size_t bpp, size_t width, size_t height)
{
	if (dststride >= 0 && dststride == srcstride)
	{
		memcpy(dst, src, srcstride * height);
	}
	else
	{
		BYTE* d = (BYTE*)dst;
		BYTE* s = (BYTE*)src;
		for (size_t i = 0; i < height; ++i)
		{
			memcpy(d, s, bpp * width);
			d += dststride;
			s += srcstride;
		}
	}
}

//==========================================================================================================================

HBITMAP BitmapLoad(IStream* stream, const SIZE* sz) throw()
{
	Image image(stream);
	if (image.GetLastStatus() != Ok)
		return NULL;
	return BitmapLoad(&image, sz);
}

HBITMAP BitmapLoad(PCWSTR filename, const SIZE* sz) throw()
{
	Image image(filename);
	if (image.GetLastStatus() != Ok)
		return NULL;
	return BitmapLoad(&image, sz);
}

HBITMAP BitmapLoad(Image* image, const SIZE* sz) throw()
{
	int		w = image->GetWidth();
	int		h = image->GetHeight();
	if (w <= 0 || h <= 0)
		return null;

	if (sz && (sz->cx < w || sz->cy < h))
	{
		double	sq = sz->cx * sz->cy;

		//　面積一定
		if (w > h)
		{
			int	ww = math::min(sz->cx * 2, (long) math::sqrt(sq * w / h));
			h = ww * h / w;
			w = ww;
		}
		else
		{
			int	hh = math::min(sz->cy * 2, (long) math::sqrt(sq * h / w));
			w = hh * w / h;
			h = hh;
		}
	}

	// 非常に汚い画像になる場合があるので GetThumbnailImage は使わない

	RECT	rc = { 0, 0, w, h };

	HDC dcScreen = GetDC(NULL);
	HBITMAP bitmap = CreateCompatibleBitmap(dcScreen, w, h);
	HDC dc = CreateCompatibleDC(dcScreen);
	ReleaseDC(NULL, dcScreen);

	HGDIOBJ old = SelectObject(dc, bitmap);

	FillRect(dc, &rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));
	Graphics(dc).DrawImage(image, 0, 0, w, h);

	SelectObject(dc, old);
	DeleteDC(dc);

	return bitmap;
}

//==========================================================================================================================

HIMAGELIST ImageListLoad(IStream* stream) throw()
{
	HIMAGELIST imagelist = NULL;
	Bitmap* image = new Bitmap(stream);
	if (image->GetLastStatus() == Ok)
		imagelist = ImageListLoad(image);
	delete image;
	return imagelist;
}

HIMAGELIST ImageListLoad(PCWSTR filename) throw()
{
	HIMAGELIST imagelist = NULL;
	Bitmap* image = new Bitmap(filename);
	if (image->GetLastStatus() == Ok)
		imagelist = ImageListLoad(image);
	delete image;
	return imagelist;
}

HIMAGELIST ImageListLoad(Bitmap* image)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	HIMAGELIST imagelist = ImageList_Create(h, h, ILC_COLOR32, w/h, 20);
	ImageList_SetBkColor(imagelist, CLR_NONE);

	if (image->GetPixelFormat() == PixelFormat32bppARGB)
	{
		Rect rcLock(0, 0, w, h);
		BitmapData bits;
		if (image->LockBits(&rcLock, ImageLockModeRead, PixelFormat32bppARGB, &bits) == Ok)
		{
			// TODO: ビットマップのコピーを減らすべし。
			// 現在、GDI+ Bitmap → DIBSection → ImageList内バッファ と複数回のコピーが発生している。
			// DIBSection をスキップできれば、コピーを減らせる。そのためには以下のどちらかの方法がある。
			// ・GDI+ Bitmapの内部データを直接参照するDIBitmapを作成する。
			// ・ImageList内のバッファに直接書き込む。
			const int bpp = 4; // sizeof(pixel for PixelFormat32bppARGB)
			BITMAPINFO info = { 0 };
			info.bmiHeader.biSize			= sizeof(info.bmiHeader);
			info.bmiHeader.biWidth			= w; 
			info.bmiHeader.biHeight			= h; 
			info.bmiHeader.biPlanes			= 1; 
			info.bmiHeader.biBitCount		= 32; 
			info.bmiHeader.biCompression	= BI_RGB;
			BYTE* dst = NULL;
			int dststride = ((w * bpp + 3) & ~3);

			HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void**)&dst, NULL, 0);
			CopyBits(dst + dststride * (h-1), -dststride, bits.Scan0, bits.Stride, bpp, w, h);
			image->UnlockBits(&bits);
			ImageList_Add(imagelist, bitmap, NULL);
			DeleteObject(bitmap);
			return imagelist;
		}
	}

	// アルファなしビットマップ
	HBITMAP bitmap;
	if (image->GetHBITMAP(SysColor(COLOR_3DFACE), &bitmap) != Ok)
		return NULL;
	ImageList_Add(imagelist, bitmap, NULL);
	DeleteObject(bitmap);
	return imagelist;
}
