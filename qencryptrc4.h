#ifndef QENCRYPTRC4_H
#define QENCRYPTRC4_H


#include <QByteArray>

class QEncryptRc4
{
    const static int sbox_len = 256;
public:
    QEncryptRc4() ;
    //void SetKey(unsigned char * key,int keylen);
    void UseKey(QString key);
    //void Encrypt(const unsigned char * source,unsigned char * target,int len);
    void Encrypt(const QByteArray & source,QByteArray & target);
private:
    void init_sbox(void);
public:
    unsigned char sbox[sbox_len];
    unsigned char kbox[sbox_len];
};

#endif // QENCRYPTRC4_H
