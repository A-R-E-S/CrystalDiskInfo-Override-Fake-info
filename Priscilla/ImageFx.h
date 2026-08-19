#pragma once
#include <windows.h>
#include <tchar.h>

#include "stb_image.h"
#include "stb_image_write.h"

class CImage
{
public:
	enum
	{
		createAlphaChannel = 0x01
	};

	CImage()
	{
		Init();
	}

	~CImage()
	{
		Destroy();
	}

	operator HBITMAP() const { return m_hBitmap; }
	BOOL IsNull() const { return m_hBitmap == NULL; }

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetPitch() const { return m_pitch; }
	int GetBPP() const { return 32; }
	void* GetBits() const { return m_bits; }

	BOOL Create(int w, int h, int bpp, DWORD flags = 0)
	{
		Destroy();

		if (bpp != 32) return FALSE;

		BITMAPINFO bmi = {};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = w;
		bmi.bmiHeader.biHeight = -h;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* bits = NULL;

		m_hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
		if (!m_hBitmap) return FALSE;

		m_bits = (BYTE*)bits;
		m_width = w;
		m_height = h;
		m_pitch = w * 4;

		m_hasAlpha = (flags & createAlphaChannel) != 0;

		memset(m_bits, 0, w * h * 4);

		return TRUE;
	}

	void Destroy()
	{
		ReleaseDC();

		if (m_hBitmap)
		{
			DeleteObject(m_hBitmap);
			m_hBitmap = NULL;
		}

		Init();
	}

	BOOL Load(LPCTSTR fileName)
	{
		Destroy();

		HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}			

		DWORD size = GetFileSize(hFile, NULL);
		BYTE* buf = (BYTE*)malloc(size);

		DWORD read = 0;
		if (ReadFile(hFile, buf, size, &read, NULL) == FALSE || read != size)
		{
			free(buf);
			CloseHandle(hFile);
			return FALSE;
		}

		BOOL result = FALSE;

		if (IsPNG(buf))
		{
			result = LoadPNG(buf, size);
		}	
		else if (IsBMP(buf))
		{
			result = LoadBMP(buf, size);
		}			

		free(buf);
		return result;
	}

	static void png_write_func(void* context, void* data, int size)
	{
		HANDLE hFile = (HANDLE)context;
		DWORD written = 0;
		WriteFile(hFile, data, size, &written, NULL);
	}

	BOOL Save(LPCTSTR fileName)
	{
		if (!m_bits || !fileName)
			return FALSE;

		if (IsPNGFileName(fileName))
			return SavePNG(fileName);

		if (IsBMPFileName(fileName))
			return SaveBMP(fileName);

		return FALSE;
	}

	BOOL IsPNGFileName(LPCTSTR fileName)
	{
		LPCTSTR ext = _tcsrchr(fileName, _T('.'));
		if (!ext) return FALSE;
		return _tcsicmp(ext, _T(".png")) == 0;
	}

	BOOL IsBMPFileName(LPCTSTR fileName)
	{
		LPCTSTR ext = _tcsrchr(fileName, _T('.'));
		if (!ext) return FALSE;
		return _tcsicmp(ext, _T(".bmp")) == 0;
	}

	BOOL SavePNG(LPCTSTR fileName)
	{
		HANDLE hFile = CreateFile(
			fileName,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		size_t size = (size_t)m_width * m_height * 4;
		BYTE* tmp = (BYTE*)malloc(size);
		if (!tmp) {
			CloseHandle(hFile);
			return FALSE;
		}

		for (int y = 0; y < m_height; y++)
		{
			BYTE* src = m_bits + y * m_pitch;
			BYTE* dst = tmp + y * m_width * 4;

			for (int x = 0; x < m_width; x++)
			{
				dst[x * 4 + 0] = src[x * 4 + 2];
				dst[x * 4 + 1] = src[x * 4 + 1];
				dst[x * 4 + 2] = src[x * 4 + 0];
				dst[x * 4 + 3] = 255;
			}
		}

		int result = stbi_write_png_to_func(
			png_write_func,
			(void*)hFile,
			m_width,
			m_height,
			4,
			tmp,
			m_width * 4);

		free(tmp);
		CloseHandle(hFile);

		return result != 0;
	}

	BOOL SaveBMP(LPCTSTR fileName)
	{
		HANDLE hFile = CreateFile(
			fileName,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		int rowSize = ((m_width * 3 + 3) / 4) * 4;
		int dataSize = rowSize * m_height;

		BITMAPFILEHEADER bf = {};
		bf.bfType = 0x4D42; // 'BM'
		bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bf.bfSize = bf.bfOffBits + dataSize;

		BITMAPINFOHEADER bi = {};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = m_width;
		bi.biHeight = m_height; // bottom-up
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;

		DWORD written = 0;

		WriteFile(hFile, &bf, sizeof(bf), &written, NULL);
		WriteFile(hFile, &bi, sizeof(bi), &written, NULL);

		BYTE* line = (BYTE*)malloc(rowSize);
		if (!line)
		{
			CloseHandle(hFile);
			return FALSE;
		}

		for (int y = m_height - 1; y >= 0; y--)
		{
			BYTE* src = m_bits + y * m_pitch;
			BYTE* dst = line;

			memset(line, 0, rowSize);

			for (int x = 0; x < m_width; x++)
			{
				dst[0] = src[0]; // B
				dst[1] = src[1]; // G
				dst[2] = src[2]; // R

				src += 4;
				dst += 3;
			}

			if (!WriteFile(hFile, line, rowSize, &written, NULL) || written != (DWORD)rowSize)
			{
				free(line);
				CloseHandle(hFile);
				return FALSE;
			}
		}

		free(line);
		CloseHandle(hFile);
		return TRUE;
	}

	BOOL BitBlt(HDC hDest, int x, int y, DWORD rop = SRCCOPY) const
	{
		return BitBlt(hDest, x, y, m_width, m_height, 0, 0, rop);
	}

	BOOL BitBlt(HDC hDest, int x, int y, int w, int h, int sx, int sy, DWORD rop) const
	{
		HDC hSrc = GetDC();

		BOOL r = ::BitBlt(hDest, x, y, w, h, hSrc, sx, sy, rop);

		ReleaseDC();
		return r;
	}

	COLORREF GetPixel(int x, int y) const
	{
		BYTE* p = m_bits + y * m_pitch + x * 4;
		return RGB(p[2], p[1], p[0]);
	}

	HDC GetDC() const
	{
		if (!m_hDC)
		{
			m_hDC = CreateCompatibleDC(NULL);
			m_hOldBitmap = (HBITMAP)SelectObject(m_hDC, m_hBitmap);
		}
		return m_hDC;
	}

	void ReleaseDC() const
	{
		if (m_hDC)
		{
			SelectObject(m_hDC, m_hOldBitmap);
			DeleteDC(m_hDC);
			m_hDC = NULL;
			m_hOldBitmap = NULL;
		}
	}

	void Attach(HBITMAP hBitmap)
	{
		Destroy();

		m_hBitmap = hBitmap;

		BITMAP bm;
		GetObject(hBitmap, sizeof(bm), &bm);

		m_width = bm.bmWidth;
		m_height = bm.bmHeight;
		m_pitch = bm.bmWidthBytes;
		m_bits = (BYTE*)bm.bmBits;

		m_hasAlpha = (bm.bmBitsPixel == 32);
	}

	HBITMAP Detach()
	{
		HBITMAP h = m_hBitmap;
		Init();
		return h;
	}

private:
	void Init()
	{
		m_hBitmap = NULL;
		m_bits = NULL;
		m_width = m_height = m_pitch = 0;
		m_hDC = NULL;
		m_hOldBitmap = NULL;
		m_hasAlpha = false;
	}

	BOOL IsPNG(BYTE* p)
	{
		static BYTE sig[8] = { 137,80,78,71,13,10,26,10 };
		return memcmp(p, sig, 8) == 0;
	}

	BOOL IsBMP(BYTE* p)
	{
		return p[0] == 'B' && p[1] == 'M';
	}

	BOOL LoadPNG(BYTE* data, int size)
	{
		int w, h, c;
		unsigned char* img = stbi_load_from_memory(data, size, &w, &h, &c, 4);
		if (!img) return FALSE;

		Create(w, h, 32, createAlphaChannel);

		for (int i = 0; i < w * h; i++)
		{
			BYTE r = img[i * 4 + 0];
			BYTE g = img[i * 4 + 1];
			BYTE b = img[i * 4 + 2];
			BYTE a = img[i * 4 + 3];

			m_bits[i * 4 + 0] = b;
			m_bits[i * 4 + 1] = g;
			m_bits[i * 4 + 2] = r;
			m_bits[i * 4 + 3] = a;
		}

		stbi_image_free(img);
		return TRUE;
	}

	BOOL LoadBMP(BYTE* data, int)
	{
		BITMAPFILEHEADER* bf = (BITMAPFILEHEADER*)data;
		BITMAPINFOHEADER* bi = (BITMAPINFOHEADER*)(data + sizeof(BITMAPFILEHEADER));

		BYTE* src = data + bf->bfOffBits;

		Create(bi->biWidth, abs(bi->biHeight), 32);

		for (int y = 0; y < m_height; y++)
		{
			BYTE* d = m_bits + y * m_pitch;
			BYTE* s = src + (m_height - 1 - y) * ((m_width * 3 + 3) & ~3);

			for (int x = 0; x < m_width; x++)
			{
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = 255;
				d += 4; s += 3;
			}
		}
		return TRUE;
	}

private:
	HBITMAP m_hBitmap;
	BYTE* m_bits;
	int     m_width, m_height, m_pitch;

	mutable HDC m_hDC;
	mutable HBITMAP m_hOldBitmap;

	bool m_hasAlpha;
};