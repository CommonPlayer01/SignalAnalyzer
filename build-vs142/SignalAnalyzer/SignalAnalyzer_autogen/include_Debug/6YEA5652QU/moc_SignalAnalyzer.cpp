/****************************************************************************
** Meta object code from reading C++ file 'SignalAnalyzer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../SignalAnalyzer/include/SignalAnalyzer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SignalAnalyzer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SignalAnalyzer_t {
    QByteArrayData data[24];
    char stringdata0[297];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SignalAnalyzer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SignalAnalyzer_t qt_meta_stringdata_SignalAnalyzer = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SignalAnalyzer"
QT_MOC_LITERAL(1, 15, 14), // "onImportFolder"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 13), // "onFileChanged"
QT_MOC_LITERAL(4, 45, 5), // "index"
QT_MOC_LITERAL(5, 51, 15), // "onSignalChanged"
QT_MOC_LITERAL(6, 67, 15), // "onWindowChanged"
QT_MOC_LITERAL(7, 83, 17), // "onComputeFinished"
QT_MOC_LITERAL(8, 101, 13), // "ComputeResult"
QT_MOC_LITERAL(9, 115, 6), // "result"
QT_MOC_LITERAL(10, 122, 14), // "onScanFinished"
QT_MOC_LITERAL(11, 137, 10), // "ScanResult"
QT_MOC_LITERAL(12, 148, 13), // "onClearCurves"
QT_MOC_LITERAL(13, 162, 14), // "onOpenSelfTest"
QT_MOC_LITERAL(14, 177, 19), // "onSelfTestRequested"
QT_MOC_LITERAL(15, 197, 4), // "freq"
QT_MOC_LITERAL(16, 202, 3), // "amp"
QT_MOC_LITERAL(17, 206, 2), // "fs"
QT_MOC_LITERAL(18, 209, 1), // "n"
QT_MOC_LITERAL(19, 211, 13), // "overlayToPlot"
QT_MOC_LITERAL(20, 225, 26), // "onSelfTestOverlayCancelled"
QT_MOC_LITERAL(21, 252, 16), // "onOverlayToggled"
QT_MOC_LITERAL(22, 269, 7), // "checked"
QT_MOC_LITERAL(23, 277, 19) // "onPointLimitChanged"

    },
    "SignalAnalyzer\0onImportFolder\0\0"
    "onFileChanged\0index\0onSignalChanged\0"
    "onWindowChanged\0onComputeFinished\0"
    "ComputeResult\0result\0onScanFinished\0"
    "ScanResult\0onClearCurves\0onOpenSelfTest\0"
    "onSelfTestRequested\0freq\0amp\0fs\0n\0"
    "overlayToPlot\0onSelfTestOverlayCancelled\0"
    "onOverlayToggled\0checked\0onPointLimitChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SignalAnalyzer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x08 /* Private */,
       3,    1,   75,    2, 0x08 /* Private */,
       5,    1,   78,    2, 0x08 /* Private */,
       6,    1,   81,    2, 0x08 /* Private */,
       7,    1,   84,    2, 0x08 /* Private */,
      10,    1,   87,    2, 0x08 /* Private */,
      12,    0,   90,    2, 0x08 /* Private */,
      13,    0,   91,    2, 0x08 /* Private */,
      14,    5,   92,    2, 0x08 /* Private */,
      20,    0,  103,    2, 0x08 /* Private */,
      21,    1,  104,    2, 0x08 /* Private */,
      23,    1,  107,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 11,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Int, QMetaType::Bool,   15,   16,   17,   18,   19,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::Int,   18,

       0        // eod
};

void SignalAnalyzer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SignalAnalyzer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onImportFolder(); break;
        case 1: _t->onFileChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onSignalChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onWindowChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->onComputeFinished((*reinterpret_cast< const ComputeResult(*)>(_a[1]))); break;
        case 5: _t->onScanFinished((*reinterpret_cast< const ScanResult(*)>(_a[1]))); break;
        case 6: _t->onClearCurves(); break;
        case 7: _t->onOpenSelfTest(); break;
        case 8: _t->onSelfTestRequested((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< bool(*)>(_a[5]))); break;
        case 9: _t->onSelfTestOverlayCancelled(); break;
        case 10: _t->onOverlayToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->onPointLimitChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ComputeResult >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ScanResult >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SignalAnalyzer::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_SignalAnalyzer.data,
    qt_meta_data_SignalAnalyzer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SignalAnalyzer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SignalAnalyzer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SignalAnalyzer.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int SignalAnalyzer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
