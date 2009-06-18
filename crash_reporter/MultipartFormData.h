#ifndef MULTIPART_FORM_DATA_H_
#define MULTIPART_FORM_DATA_H_

#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QHttpHeader>

class MultipartFormData
{
public:
	MultipartFormData();
	
	void addParameter(QString const& name, QString const& value);
	
	void addFile(QString const& name, QString const& filename, QByteArray const& data);
	
	/**
	 * \brief Set the necessary headers and return the body data.
	 *
	 * This is a destructive function and no other functions may be called after it.
	 */
	QByteArray finalize(QHttpHeader& headers);
private:
	static void generateBoundary(QByteArray& boundary);
	
	QByteArray m_boundary;
	QByteArray m_body;
};

#endif
