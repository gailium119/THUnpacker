﻿#include "stdafx.h"
#include "TH10To16Unpacker.h"
using namespace std;


// TH10To16Unpacker //////////////////////////////////////////////////////////////

TH10To16Unpacker::TH10To16Unpacker(ifstream& _f, wchar_t* _dirName, DWORD _magicNumber, const BYTE* _decParam) :
	THUnpackerBase(_f),
	magicNumber(_magicNumber),
	decParam(_decParam)
{
	dirName = _dirName;
}

void TH10To16Unpacker::ReadHeader()
{
	DWORD header[4];
	header[0] = magicNumber;
	f.read((char*)&header[1], 12);
	// Decrypt
	THDecrypt((BYTE*)header, 16, 27, 55, 16, 16);
	count = header[3] - 135792468;
	indexAddress = fileSize - (header[2] - 987654321);
	originalIndexSize = header[1] - 123456789;
}

void TH10To16Unpacker::ReadIndex()
{
	f.seekg(indexAddress);
	DWORD indexSize = fileSize - indexAddress;
	auto indexBuffer = make_unique<BYTE[]>(indexSize);
	f.read((char*)indexBuffer.get(), indexSize);
	// Decrypt
	THDecrypt(indexBuffer.get(), indexSize, 62, -101, 128, indexSize);
	// Uncompress
	indexBuffer = THUncompress(indexBuffer.get(), indexSize, originalIndexSize);
	indexSize = originalIndexSize;

	// Format index
	FormatIndex(index, indexBuffer.get(), count, indexAddress);
}

void TH10To16Unpacker::FormatIndex(vector<Index>& index, const BYTE* indexBuffer, int fileCount, DWORD indexAddress)
{
	index.resize(fileCount);
	for (int i = 0; i < fileCount; i++)
	{
		index[i].name = (char*)indexBuffer;
		DWORD offset = index[i].name.size() + 1;
		if (offset % 4 != 0)
			offset += 4 - offset % 4;
		indexBuffer += offset;
		index[i].address = ((DWORD*)indexBuffer)[0];
		index[i].originalLength = ((DWORD*)indexBuffer)[1];
		indexBuffer += 12;

		printf("%30s  %10d  %10d\n", index[i].name.c_str(), index[i].address, index[i].originalLength);
	}
	int i;
	for (i = 0; i < fileCount - 1; i++)
		index[i].length = index[i + 1].address - index[i].address;
	index[i].length = indexAddress - index[i].address;
}

bool TH10To16Unpacker::OnUncompress(const Index& index, unique_ptr<BYTE[]>& buffer, DWORD& size)
{
	BYTE asciiSum = 0;
	for (const char c : index.name)
		asciiSum += (BYTE)c;
	BYTE paramIndex = asciiSum % 8 * 3;

	THDecrypt(
		buffer.get(),
		size,
		(decParam + 0x0)[4 * paramIndex],
		(decParam + 0x1)[4 * paramIndex],
		*(int*)&(decParam + 0x4)[4 * paramIndex],
		*(int*)&(decParam + 0x8)[4 * paramIndex]
	);

	return size != index.originalLength;
}


// Child unpackers //////////////////////////////////////////////////////////////

// See th10.exe.00474BD8
const BYTE TH10Unpacker::_decParam[] = {
	0x1B, 0x37, 0xAA, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x51, 0xE9, 0xBB, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0xC1, 0x51, 0xCC, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x32, 0x00, 0x00, 0x03, 0x19, 0xDD, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00,
	0xAB, 0xCD, 0xEE, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x12, 0x34, 0xFF, 0x00,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x35, 0x97, 0x11, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x28, 0x00, 0x00, 0x99, 0x37, 0x77, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00
};

TH10Unpacker::TH10Unpacker(ifstream& _f) : 
	TH10To16Unpacker(_f, L"th10", 0xB0B35513, _decParam)
{
}

// See th11.exe.004A3480
const BYTE TH11Unpacker::_decParam[] = {
	0x1B, 0x37, 0xAA, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x51, 0xE9, 0xBB, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0xC1, 0x51, 0xCC, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x32, 0x00, 0x00, 0x03, 0x19, 0xDD, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00,
	0xAB, 0xCD, 0xEE, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x12, 0x34, 0xFF, 0x00,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x35, 0x97, 0x11, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x28, 0x00, 0x00, 0x99, 0x37, 0x77, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00
};

TH11Unpacker::TH11Unpacker(ifstream& _f) :
	TH10To16Unpacker(_f, L"th11", 0xB2B35A13, _decParam)
{
}

// See th12.exe.004AE530
const BYTE TH12Unpacker::_decParam[] = {
	0x1B, 0x73, 0xAA, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x51, 0x9E, 0xBB, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xC1, 0x15, 0xCC, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x2C, 0x00, 0x00, 0x03, 0x91, 0xDD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
	0xAB, 0xDC, 0xEE, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x12, 0x43, 0xFF, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x35, 0x79, 0x11, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x3C, 0x00, 0x00, 0x99, 0x7D, 0x77, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00
};

TH12Unpacker::TH12Unpacker(ifstream& _f) :
	TH10To16Unpacker(_f, L"th12", 0xB1B35A13, _decParam)
{
}

// See th13.exe.004BACC8
const BYTE TH13Unpacker::_decParam[] = {
	0x1B, 0x73, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x12, 0x43, 0xFF, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x35, 0x79, 0x11, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x3C, 0x00, 0x00, 0x03, 0x91, 0xDD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
	0xAB, 0xDC, 0xEE, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x51, 0x9E, 0xBB, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xC1, 0x15, 0xCC, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x2C, 0x00, 0x00, 0x99, 0x7D, 0x77, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00
};

TH13Unpacker::TH13Unpacker(ifstream& _f) :
	TH10To16Unpacker(_f, L"th13", 0xB3B35A13, _decParam)
{
}

// See th14.exe.004D1DA0
const BYTE TH14Unpacker::_decParam[] = {
	0x1B, 0x73, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x12, 0x43, 0xFF, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x35, 0x79, 0x11, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x3C, 0x00, 0x00, 0x03, 0x91, 0xDD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
	0xAB, 0xDC, 0xEE, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x51, 0x9E, 0xBB, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xC1, 0x15, 0xCC, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x2C, 0x00, 0x00, 0x99, 0x7D, 0x77, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00
};

TH14Unpacker::TH14Unpacker(ifstream& _f) :
	TH10To16Unpacker(_f, L"th14", 0xB4B35A13, _decParam)
{
}

// See th15.exe.004E0EF8, th16.exe.0049F278
const BYTE TH1516Unpacker::_decParam[] = {
	0x1B, 0x73, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x12, 0x43, 0xFF, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x35, 0x79, 0x11, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x3C, 0x00, 0x00, 0x03, 0x91, 0xDD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
	0xAB, 0xDC, 0xEE, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x51, 0x9E, 0xBB, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xC1, 0x15, 0xCC, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x2C, 0x00, 0x00, 0x99, 0x7D, 0x77, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00
};

TH1516Unpacker::TH1516Unpacker(ifstream& _f) :
	TH10To16Unpacker(_f, L"th1516", 0xB3B35A13, _decParam)
{
}
