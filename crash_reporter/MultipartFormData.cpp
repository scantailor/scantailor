#include "MultipartFormData.h"
#include <stdlib.h>

MultipartFormData::MultipartFormData()
{
	generateBoundary(m_boundary);
}

QString
MultipartFormData::contentType() const
{
	return "multipart/form-data; boundary="+QString::fromLatin1(m_boundary);
}

void
MultipartFormData::addParameter(QString const& name, QString const& value)
{
	m_body.append("--");
	m_body.append(m_boundary);
	m_body.append("\r\n");
	
	m_body.append("Content-Disposition: form-data; name=\"");
	m_body.append(name.toUtf8());
	m_body.append("\"\r\n\r\n");
	m_body.append(value.toUtf8());
	m_body.append("\r\n");
}

void
MultipartFormData::addFile(QString const& name,
	QString const& filename, QByteArray const& data)
{
	m_body.append("--");
	m_body.append(m_boundary);
	m_body.append("\r\n");
	
	m_body.append("Content-Disposition: form-data; name=\"");
	m_body.append(name.toUtf8());
	m_body.append("\"; filename=\"");
	m_body.append(filename.toUtf8());
	m_body.append("\"\r\n");
	m_body.append("Content-Type: application/octet-stream\r\n\r\n");
	m_body.append(data);
	m_body.append("\r\n");
}

QByteArray
MultipartFormData::finalize()
{
	m_body.append("--");
	m_body.append(m_boundary);
	m_body.append("--\r\n");
	return m_body;
}

void
MultipartFormData::generateBoundary(QByteArray& boundary)
{
	static char const safe_chars[64] = {
		0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
		0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
		0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
		0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
		0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
		0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33,
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x41
	};
	
	for (int i = 0; i < 8; ++i) {
		int const rnd = rand();
		for (int shift = 0; shift <= 24; shift += 8) {
			boundary.append(safe_chars[(rnd >> shift) & 0x3F]);
		}
	}
}
