#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "meta_model/qgadgetlistmodel.h"

namespace qcm
{

class LrcLyricLine {
    Q_GADGET
public:
    Q_PROPERTY(qlonglong milliseconds MEMBER milliseconds FINAL)
    Q_PROPERTY(QString content MEMBER content FINAL)

    qlonglong milliseconds;
    QString   content;
};

class LrcLyric : public meta_model::QGadgetListModel<LrcLyricLine> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qlonglong currentIndex READ currentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(qlonglong position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged FINAL)
public:
    LrcLyric(QObject* = nullptr);
    ~LrcLyric();

    QString source() const;
    void    setSource(QString);

    qlonglong position() const;
    void      setPosition(qlonglong);

    qlonglong currentIndex() const;

    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void sourceChanged();

private:
    Q_SLOT void parseLrc();
    Q_SLOT void refreshIndex();
    Q_SLOT void setCurrentIndex(qlonglong);

private:
    qlonglong m_cur_idx;
    qlonglong m_position;
    QString   m_source;
};

} // namespace qcm
