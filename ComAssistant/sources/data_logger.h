#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <QObject>
#include <QVector>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QMap>

#include "xlsxdocument.h"

//type
#define     RECOVERY_LOG    0
#define     RAW_DATA_LOG    1
#define     GRAPH_DATA_LOG  2

typedef struct file_unit_s
{
    uint8_t     type;
    QFile       *file;
    QByteArray  buff;
}file_unit_t;

class Data_Logger:public QObject
{
    Q_OBJECT
public:
    explicit Data_Logger(QObject *parent = nullptr);
    ~Data_Logger();
public slots:
    void init_logger(uint8_t type, QString path);
    void append_data_logger_buff(uint8_t type, const QByteArray &data);
    void logger_buff_flush(uint8_t type);
    void clear_logger(uint8_t type);
private:
    QVector<file_unit_t> file_group;
    bool appendToXlsx(const QByteArray &buff, QString path);
signals:
};

#endif // DATA_LOGGER_H
