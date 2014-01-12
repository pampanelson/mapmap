/*
 * ProjectReader.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ProjectReader.h"
#include <sstream>
#include <iostream>
#include <string>

ProjectReader::ProjectReader(MainWindow *window) : _window(window)
{
}

bool ProjectReader::readFile(QIODevice *device)
{
  QString errorStr;
  int errorLine;
  int errorColumn;

  QDomDocument doc;
  if (!doc.setContent(device, false, &errorStr, &errorLine, &errorColumn)) {
    std::cerr << "Error: Parse error at line " << errorLine << ", "
              << "column " << errorColumn << ": "
              << qPrintable(errorStr) << std::endl;
    return false;
  }

  QDomElement root = doc.documentElement();
  if (root.tagName() != "project" || root.attribute("version") != "1.0") {
    _xml.raiseError(QObject::tr("The file is not a libremapping version 1.0 file."));
    return false;
  }

  parseProject(root);

  return (! _xml.hasError() );
}

QString ProjectReader::errorString() const
{
  return QObject::tr("%1\nLine %2, column %3")
    .arg(_xml.errorString())
    .arg(_xml.lineNumber())
    .arg(_xml.columnNumber());
}


void ProjectReader::parseProject(const QDomElement& project)
{
  QDomElement paints = project.firstChildElement("paints");
  QDomElement mappings = project.firstChildElement("mappings");

  // Parse paints.
  QDomNode paint = paints.firstChild();
  while (!paint.isNull())
  {
    parsePaint(paint.toElement());
    paint = paint.nextSibling();
  }

  // Parse mappings.
  QDomNode mapping = mappings.firstChild();
  while (!mapping.isNull())
  {
    parseMapping(mapping.toElement());
    mapping = mapping.nextSibling();
  }
}

void ProjectReader::parsePaint(const QDomElement& paint)
{
  QString paintAttrId   = paint.attribute("id", QString::number(NULL_UID));
  QString paintAttrName = paint.attribute("name", "");
  QString paintAttrType = paint.attribute("type", "");

  if (paintAttrType == "image")
  {
    QString uri = paint.firstChildElement("uri").text();
    QString x   = paint.firstChildElement("x").text();
    QString y   = paint.firstChildElement("y").text();

    uid id = _window->createImagePaint(paintAttrId.toInt(), uri, x.toFloat(), y.toFloat());
    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create image with uri %1.").arg(uri));
  }
  else
    _xml.raiseError(QObject::tr("Unsupported paint type: %1.").arg(paintAttrType));

}

void ProjectReader::parseMapping(const QDomElement& mapping)
{
  QString mappingAttrId      = mapping.attribute("id", QString::number(NULL_UID));
  QString mappingAttrPaintId = mapping.attribute("paint_id", QString::number(NULL_UID));
  QString mappingAttrType    = mapping.attribute("type", "");

  // Get destination shape.
  QDomElement dst = mapping.firstChildElement("destination");
  QList<QPointF> dstPoints;

  if (mappingAttrType == "triangle_texture")
  {
    // Parse destination triangle.
    _parseTriangle(dst, dstPoints);

    // Get / parse source shape.
    QDomElement src = mapping.firstChildElement("source");
    QList<QPointF> srcPoints;
    _parseTriangle(src, srcPoints);

    uid id = _window->createTriangleTextureMapping(mappingAttrId.toInt(), mappingAttrPaintId.toInt(), srcPoints, dstPoints);

    if (id == NULL_UID)
      _xml.raiseError(QObject::tr("Cannot create triangle texture mapping"));
  }
  else if (mappingAttrType == "mesh_texture")
  {

  }
  else
    _xml.raiseError(QObject::tr("Unsupported mapping type: %1.").arg(mappingAttrType));
}

void ProjectReader::_parseTriangle(const QDomElement& triangle, QList<QPointF>& points)
{
  // Check that the element is really a triangle.
  QString type = triangle.attribute("shape", "");
  if (type != "triangle")
    _xml.raiseError(QObject::tr("Wrong shape type for destination: %1.").arg(type));

  // Reset list of points.
  points.clear();

  // Add vertices.
  QDomNodeList vertices = triangle.childNodes();
  if (vertices.size() != 3)
    _xml.raiseError(QObject::tr("Shape has wrong number of vertices."));

  for (int i=0; i<3; i++)
    points.push_back(_parseVertex(vertices.at(i).toElement()));
}

QPointF ProjectReader::_parseVertex(const QDomElement& vertex)
{
  return QPointF(
      vertex.attribute("x", "0").toFloat(),
      vertex.attribute("y", "0").toFloat()
      );
}


//void ProjectReader::readProject()
//{
//  // FIXME: avoid asserts
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "project");
//
//  while(! _xml.atEnd() && ! _xml.hasError())
//  {
//    /* Read next element.*/
//    QXmlStreamReader::TokenType token = _xml.readNext();
//    /* If token is just StartDocument, we'll go to next.*/
//    if (token == QXmlStreamReader::StartDocument)
//    {
//      continue;
//    }
//    /* If token is StartElement, we'll see if we can read it.*/
//    else if (token == QXmlStreamReader::StartElement)
//    {
//      if (_xml.name() == "paints")
//        continue;
//      else if (_xml.name() == "paint")
//      {
//        std::cout << " * paint" << std::endl;
//        readPaint();
//      }
//      else if (_xml.name() == "mappings")
//        continue;
//      else if (_xml.name() == "mapping")
//      {
//        std::cout << " * mapping " << std::endl;
//        readMapping(); // NULL);
//      }
//      else
//      {
//        std::cout << " * skip element " << _xml.name().string() << std::endl;
//        //_xml.skipCurrentElement();
//      }
//    }
//  } // while
//  _xml.clear();
//}
//
//void ProjectReader::readMapping()
//{
//  // FIXME: we assume an Image mapping
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "mapping");
//  const QString *paint_id_attr;
//  QXmlStreamAttributes attributes = _xml.attributes();
//
//  if (attributes.hasAttribute("", "paint_id"))
//    paint_id_attr = attributes.value("", "paint_id").string();
//
//  std::cout << "   * <mapping> " << "with paint ID " << paint_id_attr << std::endl;
//
//  //QString title = _xml.readElementText();
//  //item->setText(0, title);
//}
//
//void ProjectReader::readPaint()
//{
//  // FIXME: we assume an Image mapping
//  Q_ASSERT(_xml.isStartElement() && _xml.name() == "paint");
//  const QString *paint_id_attr;
//  const QString *uri_attr;
//  const QString *typeAttrValue;
//  QXmlStreamAttributes attributes = _xml.attributes();
//
//  if (attributes.hasAttribute("", "name"))
//    paint_id_attr = attributes.value("", "name").string();
//  if (attributes.hasAttribute("", "type"))
//    typeAttrValue = attributes.value("", "type").string();
//
//  std::cout << "Found " << typeAttrValue->toStdString() <<
//    " paint " << paint_id_attr->toStdString() << std::endl;
//
//  /* Next element... */
//  _xml.readNext();
//  // /*
//  //  * We're going to loop over the things because the order might change.
//  //  * We'll continue the loop until we hit an EndElement.
//  //  */
//  while(! (_xml.tokenType() == QXmlStreamReader::EndElement &&
//    _xml.name() == "paint"))
//  {
//    if (_xml.tokenType() == QXmlStreamReader::StartElement)
//    {
//      if (_xml.name() == "uri")
//      {
//        /* ...go to the next. */
//        _xml.readNext();
//        /*
//         * This elements needs to contain Characters so we know it's
//         * actually data, if it's not we'll leave.
//         */
//        if(_xml.tokenType() != QXmlStreamReader::Characters)
//        {
//          // pass
//        }
//        //uri_attr = _xml.text().toString();
//        //std::cout << "uri " << uri_attr.toStdString() << std::endl;
//      }
//      else if (_xml.name() == "width")
//      {
//        // pass
//      }
//      else if (_xml.name() == "height")
//      {
//        // pass
//      }
//    }
//    _xml.readNext();
//  }
//
//  // TODO: call this->_manager->getController->createPaint(...)
//}
