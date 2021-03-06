#include "reg_match_engine.h"

RegMatchEngine::RegMatchEngine(QObject *parent)
    : QObject(parent)
{
    // this run in main thread.
//    qDebug()<<"hello thread";
}

RegMatchEngine::~RegMatchEngine()
{
    // this run in main thread.
//    qDebug()<<"goodbye thread";
}

void RegMatchEngine::updateRegMatch(QString newStr)
{
    if(newStr != RegMatchStr)
    {
        clearData();
        RegMatchStr = newStr;
    }
}

//好像没啥用
void RegMatchEngine::updateCodec(QString codec)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName(codec.toLocal8Bit()));
}

//从缓存中提取所有包
void RegMatchEngine::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer, QMutex &bufferLock)
{
    if(buffer.isEmpty() || buffer == "\n")
        return;
    if(RegMatchStr.isEmpty())
        return;

    QString pattern = "\n.*" + RegMatchStr + ".*\n";
    QRegularExpression reg;
    QRegularExpressionMatch match;
    int scanIndex = 0;
    int lastScannedIndex = 0;
    reg.setPattern(pattern.toLocal8Bit());
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            QByteArray onePack;
            match = reg.match(buffer, scanIndex);
            if(match.hasMatch()) {
                //-1是因为匹配借用了前面数据包结尾的换行符\n，所以保留最后的换行符供下一个数据包借用
                scanIndex = match.capturedEnd() - 1;
                lastScannedIndex = scanIndex;
                onePack = match.captured(0).toLocal8Bit();
                onePack.remove(0, 1); //remove first '\n', last
                matchedDataPool.append(onePack);
                onePack.remove(onePack.size()-1, 1);//remove last '\r\n'
                if(onePack.endsWith("\r"))
                {
                    onePack.remove(onePack.size()-1, 1);
                }
                emit dataUpdated(onePack);
            }else{
//                qDebug()<<"no match";
                if(buffer.size() - scanIndex > MAX_EXTRACT_LENGTH)
                {
                    scanIndex += MAX_EXTRACT_LENGTH;
                }
                else
                {
                    scanIndex++;
                }
            }
    } while(scanIndex < buffer.size());

    bufferLock.lock();
    restBuffer = buffer.mid(lastScannedIndex);
    bufferLock.unlock();
}

void RegMatchEngine::appendData(const QByteArray &newData)
{
    QByteArray tmp;
    //剔除正则匹配无法很好支持的字符：中文、'\0'
    foreach(char ch, newData)
    {
        if(ch & 0x80 || ch == '\0')
        {
            continue;
        }
        tmp.append(ch);
    }
    dataLock.lock();
    //由于匹配借用了前面数据包结尾的换行符，因此首个数据包要在前面补\n
    if(rawDataBuff.isEmpty())
        rawDataBuff = "\n";
    rawDataBuff.append(tmp);
    dataLock.unlock();
}

void RegMatchEngine::parseData()
{
    parsePacksFromBuffer(rawDataBuff, rawDataBuff, dataLock);
    //在解析完成后剔除前面已扫描过的数据
    if(rawDataBuff.size() > MAX_EXTRACT_LENGTH)
    {
        dataLock.lock();
        rawDataBuff = rawDataBuff.mid(rawDataBuff.size() - MAX_EXTRACT_LENGTH);
        dataLock.unlock();
    }
}

void RegMatchEngine::appendAndParseData(const QByteArray &newData)
{
    appendData(newData);
    parseData();
}

//从文本组集合中清除指定名称的文本组
void RegMatchEngine::clearData()
{
    rawDataBuff.clear();
    matchedDataPool.clear();
    //由于匹配借用了前面数据包结尾的换行符，因此这里也要保留最后的换行符供下一个数据包借用
    rawDataBuff = "\n";
}

//保存指定名字的文本组到指定路径
qint32 RegMatchEngine::saveData(const QString &path)
{
    //保存数据
    QFile file(path);
    //删除旧数据形式写文件
    if(file.open(QFile::WriteOnly|QFile::Truncate)){
        file.write(matchedDataPool);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
        file.flush();
        file.close();
        emit saveDataResult(SAVE_OK, path, file.size());
        return SAVE_OK;
    }else{
        emit saveDataResult(OPEN_FAILED, path, 0);
        return OPEN_FAILED;
    }
    emit saveDataResult(UNKNOW_NAME, path, 0);
    return UNKNOW_NAME;
}
