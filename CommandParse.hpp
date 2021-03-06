#ifndef COMMANDPARSE_H
#define COMMANDPARSE_H

#include <QString>
#include <QList>
#include <QtXml>
#include <QFile>
#include <QByteArray>
#include <QString>
#include <Swiften/Elements/IQ.h>

#include "pugixml.hpp"
#include "mainwindow.h"

#include "ClientInterface.h"
#include "session.h"

#define  TAGNAME    "CZXP"

#define  CMD_NORMAL 0
#define  CMD_START  1
#define  CMD_ERROR  -1


using namespace std;


class CommandParse
{
public:

    static bool  parseServerData(const QString src, QString &xmlns,  XMLNSList &list)
    /*
    *如果该条数据与给定的命名空间相符，那么保留该数据，转发给客户端应用
    *
    */
    {
        XMLNSList::iterator   it = list.begin();
        QDomDocument    dom;
        QDomElement     elemiq,elemmsg,elem;
        if( !dom.setContent(src) )
            return false;
        elemiq = dom.firstChildElement(QString("iq"));
        elemmsg = dom.firstChildElement(QString("message"));
        if ( elemiq.isNull() && elemmsg.isNull() )
            return false;
        elem = elemiq.isNull() ? elemmsg :elemiq;

        QDomElement tmp = elem.firstChildElement(QString(TAGNAME));
        if ( tmp.isNull() )
           return false;
        xmlns = tmp.attribute(QString("xmlns"));
        return true;

    }
    //解析随机码验证IQ，并返回认证结果

    static bool parseRandCodeIq(const QString cmd,QString identity /*, QString &tojid, QString &xmlns,QString &authcode*/)
    {
        using namespace Swift;
        QDomDocument  dom;
        QDomElement   elem;
        QString       tojid,xmlns,authcode,id;
        QString       data;

        if ( !dom.setContent(cmd )){
            return false;
        }
        //不是符合要求的IQ报文，直接丢弃
        elem = dom.firstChildElement("iq");
        tojid = elem.attribute("from");
        id = elem.attribute("id");
        QString myjid = elem.attribute("to");
        Session::Instance()->setWholeJid(myjid);
        Session::Instance()->setjid(QString::fromStdString(JID(myjid.toStdString()).toBare().toString()));
        myjid = QString::fromStdString(JID(myjid.toStdString()).getNode());
        Session::Instance()->setIdentity(myjid);
        if (elem.isNull()) return false;
        elem = elem.firstChildElement("CZXP");
        if (elem.isNull()) return false;
        xmlns = elem.attribute("xmlns");
        elem = elem.firstChildElement("userAuth");
        if(elem.isNull()) return false;
        authcode = elem.lastChild().nodeValue();
        QString realCode = Session::Instance()->getcode();
        if(realCode.compare(authcode) != 0)
            return false;

        data  = QString("<iq to=\"") + tojid + QString("\" ");
        data += QString(" id=\"")+id + QString("\" type=\"result\">");
        data += QString("<CZXP xmlns=\"") + xmlns + QString("\">");
        data += QString("<userAuth>") + Session::Instance()->getIdentity() + QString("</userAuth>");
        data += QString("</CZXP>");
        data += QString("</iq>");

        MainWindow::getInstance()->getClient()->sendData(data.toStdString());

        return true;
    }
    //请求验证的IQ
    static bool  parseVerifyIq(const QString cmd,QString &code, QString &tojid)
    {
        QDomDocument  dom;
        QDomElement   elem;
        //QString scmd = QString("<iq from =\"\",to=\"123\" type=\"result\"><xmlns=\"123\"/><code>xuojfoamdfoaysdYQOEWJN232</code></iq>");
        if ( !dom.setContent(cmd))
        {
            return false;
        }
        elem = dom.firstChildElement(QString("iq"));
        tojid = elem.attribute(QString("from"),QString(""));
        if (elem.isNull())
            return false;
        elem = elem.firstChildElement(QString("code"));
        if (elem.isNull())
            return false;
        code = elem.lastChild().nodeValue();
        return true;
    }

    //解析客户端发送的数据流
    static int     parseCmd(const QString cmd, QList<QString> &ns)
    /*
解析用户发送的命令,如果是报文,直接转发,返回值为0,否则返回1, ns 中存储命名空间列表
 开始报文的格式为
<start>
<namespace/>
<namespace/>
<namespace/>
<appname/>
 ..
</start>
*/
{
    int pos = 0;

    pos = cmd.indexOf(QString("<CZXP"));

    if ( pos < 0)
        return CMD_ERROR;
    pos = cmd.indexOf(QString("iq"));
    if ( pos >= 0) return CMD_NORMAL;
    pos = cmd.indexOf(QString("message"));
    if (pos >= 0) return CMD_NORMAL;
    //解析开始报文
    QDomDocument  dom;
    QDomElement   root;
    //QDomNode      root;
    QDomNodeList  nslist;

    if( !dom.setContent(cmd.toUtf8()))
        return CMD_ERROR;
    //如果没有start节点则认为是正常的报文
    root = dom.firstChild().toElement();
    if ( root.isNull())
        return CMD_NORMAL;
    nslist = root.elementsByTagName(QString("namespace"));
    int sz = nslist.size();
    if ( sz  == 0 )
        return CMD_NORMAL;
    ns.clear();
    for ( pos = 0; pos < sz; ++pos)
    {
        QDomNode    node = nslist.item(pos);
        QString     value = node.lastChild().nodeValue();
        ns.push_back(value);
    }
    return CMD_START;
}
    //////////////////////////////////////////////////////
   static  QString trimString(QString &str)
    {
       char sp[128];
       str = str.trimmed();
       sprintf(sp,str.toStdString().c_str());
       char *h = sp;
       while (*h != '\0')
       {
           if (*h == '\r' || *h == '\n')
               *h = '\0';
           h++;
       }
       str = QString::fromAscii(sp);
       return str;
    }
    static void test()
    {
        QFile file("test.txt");
        file.open(QIODevice::ReadOnly);
        QString str = file.readAll(),iq;
        parseVerifyIq(str,str,iq);
    }
    static QString createID()
    {
        char seed[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
                      'o','p','q','r','s','t','u','v','w','x','y','z','1','2','3',
                      '4','5','6','7','8','9'};
        int  sz = sizeof(seed);
        int   i = 0;
        QString ret;
        for ( i = 0 ;i < 5; ++i)
        {
            ret.append(seed[rand()%sz]);
        }
        return ret;
    }

    static QString createSearchIQ(QString search,QString srv)
    {
      QDomDocument  dom;
      QDomElement   elem;
      int           i = 0;
      id_           =  createID();
      elem = dom.createElement("iq");
      elem.setAttribute("type","set");
      elem.setAttribute("to",srv);
      elem.setAttribute("id",id_);

      QDomElement   query = dom.createElement("query");
      query.setAttribute("xmlns","jabber:iq:search");
      elem.appendChild(query);

      QDomElement   x  = dom.createElement("x");
      x.setAttribute("xmlns","jabber:x:data");
      x.setAttribute("type","submit");
      query.appendChild(x);

      QDomElement   field1  = dom.createElement("field");
      field1.setAttribute("type","hidden");
      field1.setAttribute("var","FORM_TYPE");
      QDomElement   value = dom.createElement("value");
      QDomText      dt = dom.createTextNode("jabber:iq:search");
      value.appendChild(dt);
      field1.appendChild(value);
      x.appendChild(field1);

      QDomElement   field[3];
      QStringList   varlist;
      varlist.append("Username");
      varlist.append("Name");
      varlist.append("Email");
      for ( i = 0 ; i < 3; ++i)
      {
          field[i]= dom.createElement("field");
          field[i].setAttribute("type","boolean");
          field[i].setAttribute("var",varlist[i]);
          QDomElement value = dom.createElement("value");
          QDomText    t = dom.createTextNode("1");
          value.appendChild(t);
          field[i].appendChild(value);
          x.appendChild(field[i]);
      }
      QDomElement  fields = dom.createElement("field");
      fields.setAttribute("type","text-single");
      fields.setAttribute("var","search");
      QDomElement  values = dom.createElement("value");
      QDomText      txt = dom.createTextNode(search);
      values.appendChild(txt);
      fields.appendChild(values);
      x.appendChild(fields);
      dom.appendChild(elem);
      QString xmls = dom.toString();
#if 0
      QFile   file("D:\\iq.txt");
      file.open(QIODevice::WriteOnly);
      file.write(xmls.toAscii());
      file.close();
#endif
      return xmls;
    }
    static bool cmpID(QString id)
    {
        return id.compare(id_);
    }

    static QString id_;
};
#endif // COMMANDPARSE_H
