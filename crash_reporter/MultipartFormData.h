#ifndef MULTIPART_FORM_DATA_H_
#define MULTIPART_FORM_DATA_H_

#include <QString>
#include <QByteArray>
#include <QDataStream>

class MultipartFormData
{
public:
	MultipartFormData();
	
	/**
	 * \brief Returns the content type that needs to be set for HTTP requests.
	 */
	QString contentType() const;

	void addParameter(QString const& name, QString const& value);
	
	void addFile(QString const& name, QString const& filename, QByteArray const& data);
	
	/**
	 * \brief Marks the end of parameters and returns the serialized
	 *        representation of the form.
	 *
	 * Once called, no other data may be added to the form.
	 */
	QByteArray finalize();
private:
	static void generateBoundary(QByteArray& boundary);
	
	QByteArray m_boundary;
	QByteArray m_body;
};

#endif
