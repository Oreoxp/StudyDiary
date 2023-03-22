/****************************************************************************
** Meta object code from reading C++ file 'fboinsgrenderer.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../fboinsgrenderer.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fboinsgrenderer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_GLFWItem_t {
    uint offsetsAndSizes[34];
    char stringdata0[9];
    char stringdata1[12];
    char stringdata2[9];
    char stringdata3[19];
    char stringdata4[1];
    char stringdata5[15];
    char stringdata6[21];
    char stringdata7[19];
    char stringdata8[6];
    char stringdata9[18];
    char stringdata10[14];
    char stringdata11[18];
    char stringdata12[11];
    char stringdata13[10];
    char stringdata14[11];
    char stringdata15[8];
    char stringdata16[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_GLFWItem_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_GLFWItem_t qt_meta_stringdata_GLFWItem = {
    {
        QT_MOC_LITERAL(0, 8),  // "GLFWItem"
        QT_MOC_LITERAL(9, 11),  // "QML.Element"
        QT_MOC_LITERAL(21, 8),  // "Renderer"
        QT_MOC_LITERAL(30, 18),  // "trianglePosChanged"
        QT_MOC_LITERAL(49, 0),  // ""
        QT_MOC_LITERAL(50, 14),  // "keyDownChanged"
        QT_MOC_LITERAL(65, 20),  // "GLFWItem::CLICK_TYPE"
        QT_MOC_LITERAL(86, 18),  // "sliderValueChanged"
        QT_MOC_LITERAL(105, 5),  // "value"
        QT_MOC_LITERAL(111, 17),  // "changeTrianglePos"
        QT_MOC_LITERAL(129, 13),  // "changeKeyDown"
        QT_MOC_LITERAL(143, 17),  // "changeSliderValue"
        QT_MOC_LITERAL(161, 10),  // "CLICK_TYPE"
        QT_MOC_LITERAL(172, 9),  // "DOWN_LEFT"
        QT_MOC_LITERAL(182, 10),  // "DOWN_RIGHT"
        QT_MOC_LITERAL(193, 7),  // "DOWN_UP"
        QT_MOC_LITERAL(201, 9)   // "DOWN_DOWN"
    },
    "GLFWItem",
    "QML.Element",
    "Renderer",
    "trianglePosChanged",
    "",
    "keyDownChanged",
    "GLFWItem::CLICK_TYPE",
    "sliderValueChanged",
    "value",
    "changeTrianglePos",
    "changeKeyDown",
    "changeSliderValue",
    "CLICK_TYPE",
    "DOWN_LEFT",
    "DOWN_RIGHT",
    "DOWN_UP",
    "DOWN_DOWN"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_GLFWItem[] = {

 // content:
      10,       // revision
       0,       // classname
       1,   14, // classinfo
       6,   16, // methods
       0,    0, // properties
       1,   66, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // classinfo: key, value
       1,    2,

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   52,    4, 0x06,    1 /* Public */,
       5,    1,   53,    4, 0x06,    2 /* Public */,
       7,    1,   56,    4, 0x06,    4 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       9,    0,   59,    4, 0x02,    6 /* Public */,
      10,    1,   60,    4, 0x02,    7 /* Public */,
      11,    1,   63,    4, 0x02,    9 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    4,
    QMetaType::Void, QMetaType::Int,    8,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    4,
    QMetaType::Void, QMetaType::Int,    8,

 // enums: name, alias, flags, count, data
      12,   12, 0x0,    4,   71,

 // enum data: key, value
      13, uint(GLFWItem::DOWN_LEFT),
      14, uint(GLFWItem::DOWN_RIGHT),
      15, uint(GLFWItem::DOWN_UP),
      16, uint(GLFWItem::DOWN_DOWN),

       0        // eod
};

Q_CONSTINIT const QMetaObject GLFWItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickFramebufferObject::staticMetaObject>(),
    qt_meta_stringdata_GLFWItem.offsetsAndSizes,
    qt_meta_data_GLFWItem,
    qt_static_metacall,
    nullptr,
    qt_metaTypeArray<
        // Q_OBJECT / Q_GADGET
        GLFWItem,
        // method 'trianglePosChanged'
        void,
        // method 'keyDownChanged'
        void,
        GLFWItem::CLICK_TYPE,
        // method 'sliderValueChanged'
        void,
        int,
        // method 'changeTrianglePos'
        void,
        // method 'changeKeyDown'
        void,
        GLFWItem::CLICK_TYPE,
        // method 'changeSliderValue'
        void,
        int
    >,
    nullptr
} };

void GLFWItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GLFWItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->trianglePosChanged(); break;
        case 1: _t->keyDownChanged((*reinterpret_cast< std::add_pointer_t<GLFWItem::CLICK_TYPE>>(_a[1]))); break;
        case 2: _t->sliderValueChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->changeTrianglePos(); break;
        case 4: _t->changeKeyDown((*reinterpret_cast< std::add_pointer_t<GLFWItem::CLICK_TYPE>>(_a[1]))); break;
        case 5: _t->changeSliderValue((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GLFWItem::*)();
            if (_t _q_method = &GLFWItem::trianglePosChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GLFWItem::*)(GLFWItem::CLICK_TYPE );
            if (_t _q_method = &GLFWItem::keyDownChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GLFWItem::*)(int );
            if (_t _q_method = &GLFWItem::sliderValueChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *GLFWItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GLFWItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GLFWItem.stringdata0))
        return static_cast<void*>(this);
    return QQuickFramebufferObject::qt_metacast(_clname);
}

int GLFWItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickFramebufferObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void GLFWItem::trianglePosChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GLFWItem::keyDownChanged(GLFWItem::CLICK_TYPE _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void GLFWItem::sliderValueChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
namespace {
struct qt_meta_stringdata_GLFWRenderer_t {
    uint offsetsAndSizes[14];
    char stringdata0[13];
    char stringdata1[21];
    char stringdata2[1];
    char stringdata3[17];
    char stringdata4[21];
    char stringdata5[21];
    char stringdata6[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_GLFWRenderer_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_GLFWRenderer_t qt_meta_stringdata_GLFWRenderer = {
    {
        QT_MOC_LITERAL(0, 12),  // "GLFWRenderer"
        QT_MOC_LITERAL(13, 20),  // "onTrianglePosChanged"
        QT_MOC_LITERAL(34, 0),  // ""
        QT_MOC_LITERAL(35, 16),  // "onKeyDownChanged"
        QT_MOC_LITERAL(52, 20),  // "GLFWItem::CLICK_TYPE"
        QT_MOC_LITERAL(73, 20),  // "onSliderValueChanged"
        QT_MOC_LITERAL(94, 5)   // "value"
    },
    "GLFWRenderer",
    "onTrianglePosChanged",
    "",
    "onKeyDownChanged",
    "GLFWItem::CLICK_TYPE",
    "onSliderValueChanged",
    "value"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_GLFWRenderer[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x0a,    1 /* Public */,
       3,    1,   33,    2, 0x0a,    2 /* Public */,
       5,    1,   36,    2, 0x0a,    4 /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    2,
    QMetaType::Void, QMetaType::Int,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject GLFWRenderer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_GLFWRenderer.offsetsAndSizes,
    qt_meta_data_GLFWRenderer,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_GLFWRenderer_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GLFWRenderer, std::true_type>,
        // method 'onTrianglePosChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onKeyDownChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<GLFWItem::CLICK_TYPE, std::false_type>,
        // method 'onSliderValueChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void GLFWRenderer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GLFWRenderer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onTrianglePosChanged(); break;
        case 1: _t->onKeyDownChanged((*reinterpret_cast< std::add_pointer_t<GLFWItem::CLICK_TYPE>>(_a[1]))); break;
        case 2: _t->onSliderValueChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *GLFWRenderer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GLFWRenderer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GLFWRenderer.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QQuickFramebufferObject::Renderer"))
        return static_cast< QQuickFramebufferObject::Renderer*>(this);
    if (!strcmp(_clname, "QOpenGLFunctions_3_0"))
        return static_cast< QOpenGLFunctions_3_0*>(this);
    return QObject::qt_metacast(_clname);
}

int GLFWRenderer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
