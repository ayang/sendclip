#include "qencryptrc4.h"
#include <QString>


QEncryptRc4::QEncryptRc4()
{

}

void QEncryptRc4::UseKey(QString key)
{
    int len = key.length();
    if(key.isEmpty()) {
        return ;
    }
    for(int i=0;i<sbox_len;i++) {
        this->kbox[i] = key.toLocal8Bit().data()[i % len];
    }
}

void QEncryptRc4::Encrypt(const QByteArray & source,QByteArray & target)
{
    int i = 0, j = 0, t, index = 0;
    unsigned char tmp;

    int len = source.size();

    target.resize(len);

    init_sbox();

    while (index < len)
    {
        i = (i + 1) % sbox_len;
        j = (j + this->sbox[i]) % sbox_len;
        tmp = this->sbox[i];
        this->sbox[i] = this->sbox[j];
        this->sbox[j] = tmp;
        t = (this->sbox[i] + this->sbox[j]) % sbox_len;

        target[index] = source[index] ^ this->sbox[t];
        index++;
    }
}

void QEncryptRc4::init_sbox(void)
{
    int i, j = 0;
    unsigned char tmp;

    for (i = 0; i < sbox_len; i++)
        this->sbox[i] = i;

    for (i = 0; i < sbox_len; i++)
    {
        j = (j + this->sbox[i] + this->kbox[i]) % sbox_len;
        tmp = this->sbox[i];
        this->sbox[i] = this->sbox[j];
        this->sbox[j] = tmp;
    }
}
