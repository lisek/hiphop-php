/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 Facebook, Inc. (http://www.facebook.com)          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include <cpp/base/type_variant.h>
#include <cpp/base/type_string.h>
#include <cpp/base/type_array.h>
#include <cpp/base/type_object.h>
#include <cpp/base/comparisons.h>
#include <cpp/base/zend/zend_functions.h>
#include <lib/system/gen/php/classes/stdclass.h>
#include <cpp/base/variable_serializer.h>
#include <cpp/base/variable_unserializer.h>
#include <cpp/base/externals.h>
#include <cpp/ext/ext_variable.h>
#include <cpp/base/runtime_option.h>

using namespace std;

namespace HPHP {

const Variant Variant::s_nullVariant = Variant();

IMPLEMENT_SMART_ALLOCATION_NOCALLBACKS(Variant);
///////////////////////////////////////////////////////////////////////////////
// private implementations

Variant::Variant(CStrRef v) : m_type(KindOfString) {
  StringData *s = v.get();
  if (s) {
    m_data.pstr = s;
    s->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(CArrRef v) : m_type(KindOfArray) {
  ArrayData *a = v.get();
  if (a) {
    m_data.parr = a;
    a->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(CObjRef v) : m_type(KindOfObject) {
  ObjectData *o = v.get();
  if (o) {
    m_data.pobj = o;
    o->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(StringData *v) : m_type(KindOfString) {
  if (v) {
    m_data.pstr = v;
    v->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(ArrayData *v) : m_type(KindOfArray) {
  if (v) {
    m_data.parr = v;
    v->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(ObjectData *v) : m_type(KindOfObject) {
  if (v) {
    m_data.pobj = v;
    v->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(Variant *v) : m_type(KindOfVariant) {
  if (v) {
    m_data.pvar = v;
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
}

Variant::Variant(CVarRef v) : m_type(KindOfNull) {
  m_data.num = 0;
  if (v.isContagious()) {
    v.clearContagious(); // do it early to avoid bind() triggers deletion of v

    // we have to wrap up v into a sharable form
    if (v.m_type != KindOfVariant) {
      Variant *shared = NEW(Variant)();
      shared->bind(v);
      const_cast<Variant&>(v).bind(shared);
    }

    // then we can share v.m_data.pvar
    bind(v.m_data.pvar);

  } else {
    bind(v);
  }
}

void Variant::reset() {
  m_data.num = 0;
  m_type = KindOfNull;
}

void Variant::destruct() {
  switch (m_type) {
  case KindOfString:
    if (m_data.pstr->decRefCount() == 0) {
      m_data.pstr->release();
    }
    break;
  case KindOfArray:
    if (m_data.parr->decRefCount() == 0) {
      m_data.parr->release();
    }
    break;
  case KindOfObject:
    if (m_data.pobj->decRefCount() == 0) {
      m_data.pobj->release();
    }
    break;
  case KindOfVariant:
    if (m_data.pvar->decRefCount() == 0) {
      m_data.pvar->release();
    }
    break;
  default:
    break;
  }
}

void Variant::setNull() {
  if (isPrimitive()) {
    m_data.num = 0;
    m_type = KindOfNull;
  } else if (m_type == KindOfVariant) {
    m_data.pvar->setNull();
  } else {
    unset();
  }
}

CVarRef Variant::set(bool v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfBoolean;
  m_data.num = (v ? 1 : 0);
  return *this;
}

CVarRef Variant::set(char v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfByte;
  m_data.num = v;
  return *this;
}

CVarRef Variant::set(short v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfInt16;
  m_data.num = v;
  return *this;
}

CVarRef Variant::set(int v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfInt32;
  m_data.num = v;
  return *this;
}

CVarRef Variant::set(int64 v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfInt64;
  m_data.num = v;
  return *this;
}

CVarRef Variant::set(double v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = KindOfDouble;
  m_data.dbl = v;
  return *this;
}

CVarRef Variant::set(litstr v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  m_type = LiteralString;
  m_data.str = v;
  return *this;
}

CVarRef Variant::set(const ObjectOffset& v) {
  return (*this = static_cast<Variant>(v));
}

CVarRef Variant::set(StringData *v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  if (v) {
    m_type = KindOfString;
    m_data.pstr = v;
    v->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
  return *this;
}

CVarRef Variant::set(ArrayData *v) {
  if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  }
  unset();
  if (v) {
    m_type = KindOfArray;
    m_data.parr = v;
    v->incRefCount();
  }
  return *this;
}

CVarRef Variant::set(ObjectData *v) {
  if (isPrimitive()) {
    // do nothing
  } else if (m_type == KindOfVariant) {
    m_data.pvar->set(v);
    return *this;
  } else {
    unset();
  }
  if (v) {
    m_type = KindOfObject;
    m_data.pobj = v;
    v->incRefCount();
  } else {
    m_data.num = 0;
    m_type = KindOfNull;
  }
  return *this;
}

CVarRef Variant::set(CStrRef v) {
  return set(v.get());
}

CVarRef Variant::set(CArrRef v) {
  return set(v.get());
}

CVarRef Variant::set(CObjRef v) {
  return set(v.get());
}

void Variant::escalateString() const {
  if (m_type == KindOfVariant) {
    m_data.pvar->escalateString();
    return;
  }
  ASSERT(getType() == LiteralString);
  m_type = KindOfString;
  StringData *v = NEW(StringData)(m_data.str);
  m_data.pstr = v;
  v->incRefCount();
}

// This function should only be called from bind(CVarRef v) because they are
// tightly coupled together. In particular m_type should be viewed as
// KindOfNull and for complex types the old data already released.
inline void Variant::bind2(CVarRef v) {
  switch (v.m_type) {
  // copy-on-write: ref counting complex types
  case KindOfString: {
    StringData *str = v.m_data.pstr;
    if (str) {
      m_type = KindOfString;
      m_data.pstr = str;
      str->incRefCount();
    } else {
      m_data.num = 0;
      m_type = KindOfNull;
    }
    break;
  }
  case KindOfArray: {
    ArrayData *arr = v.m_data.parr;
    if (arr) {
      m_type = KindOfArray;
      m_data.parr = arr;
      arr->incRefCount();
    } else {
      m_data.num = 0;
      m_type = KindOfNull;
    }
    break;
  }
  case KindOfObject: {
    ObjectData *obj = v.m_data.pobj;
    if (obj) {
      m_type = KindOfObject;
      m_data.pobj = obj;
      obj->incRefCount();
    } else {
      m_data.num = 0;
      m_type = KindOfNull;
    }
    break;
  }
  case KindOfVariant:
    m_type = KindOfNull;
    bind(*v.m_data.pvar);
    break;
  default:
    ASSERT(false);
  }
}

void Variant::bind(CVarRef v) {
  if (isPrimitive()) {
    if (v.isPrimitive()) {
      m_type = v.m_type;
      m_data = v.m_data;
    } else {
      bind2(v);
    }
  } else {
    switch (m_type) {
    case KindOfString:
    case KindOfArray:
    case KindOfObject:
      destruct();
      m_type = KindOfNull;
      if (v.isPrimitive()) {
        m_type = v.m_type;
        m_data = v.m_data;
      } else {
        bind2(v);
      }
      break;
    case KindOfVariant:
      m_data.pvar->bind(v);
      break;
    default:
      ASSERT(false);
    }
  }
}

void Variant::bind(Variant *v) {
  ASSERT(v != this);
  if (v) {
    v->incRefCount(); // in case unset() triggers deletion of v
    unset();
    m_type = KindOfVariant;
    m_data.pvar = v;
  } else {
    unset();
  }
}

void Variant::copy(CVarRef v, bool deep) {
  // in case copying myself...
  if (m_type == v.m_type && m_data.num == v.m_data.num) {
    return;
  }

  // if we are in StrongBinding, we copy the value into the shared data
  if (m_type == KindOfVariant) {
    m_data.pvar->copy(v, deep);
    return;
  }

  switch (v.m_type) {

  // copy primitive values
  case KindOfNull:    unset();                  break;
  case KindOfBoolean: set(v.m_data.num != 0);   break;
  case KindOfByte:    set((char)v.m_data.num);  break;
  case KindOfInt16:   set((short)v.m_data.num); break;
  case KindOfInt32:   set((int)v.m_data.num);   break;
  case KindOfInt64:   set(v.m_data.num);        break;
  case KindOfDouble:  set(v.m_data.dbl);        break;
  case LiteralString: set(v.m_data.str);        break;

  // copy-by-value
  case KindOfString:  set(v.m_data.pstr->copy()); break;
  case KindOfArray:   set(v.m_data.parr->copy()); break;

  // objects are never copied by value
  case KindOfObject:  set(v.m_data.pobj);   break;

  // go one and only one level deeper
  case KindOfVariant:
    if (deep) {
      copy(*v.m_data.pvar, true);
    } else {
      bind(v.m_data.pvar);
    }
    break;

  default:
    ASSERT(false);
    break;
  }
}

void Variant::split() {
  switch (getType()) {
  case KindOfVariant: m_data.pvar->split();     break;
  // copy-on-write
  case KindOfString:  set(m_data.pstr->copy()); break;
  case KindOfArray:   set(m_data.parr->copy()); break;
  default:
    break;
  }
}

Variant &Variant::assign(CVarRef v, bool deep) {
  ASSERT(!isContagious() ||
         this == &v); // otherwise our code generation is wrong
  if (v.isContagious()) {
    v.clearContagious(); // do it early to avoid bind() triggers deletion of v

    // we have to wrap up v into a sharable form
    if (v.m_type != KindOfVariant) {
      Variant *shared = NEW(Variant)();
      shared->bind(v);
      const_cast<Variant&>(v).bind(shared);
    }

    // then we can share v.m_data.pvar
    bind(v.m_data.pvar);

  } else if (isReferenced()) {
    m_data.pvar->assign(v, deep);
  } else {
    if (this != &v) {
      bind(v);
    }
  }
  return *this;
}

int Variant::getRefCount() const {
  switch (m_type) {
  case KindOfString:  return m_data.pstr->getCount();
  case KindOfArray:   return m_data.parr->getCount();
  case KindOfObject:  return m_data.pobj->getCount();
  case KindOfVariant: return m_data.pvar->getRefCount();
  default:
    break;
  }
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// informational

bool Variant::isInteger() const {
  switch (m_type) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    return true;
  case KindOfVariant:
    return m_data.pvar->isInteger();
  default:
    break;
  }
  return false;
}

bool Variant::isNumeric(bool checkString /* = false */) const {
  switch (m_type) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
  case KindOfDouble:
    return true;
  case LiteralString:
    if (checkString) {
      const char *data = m_data.str;
      int len = strlen(data);
      if (len) {
        int64 lval; double dval;
        DataType ret = is_numeric_string(data, len, &lval, &dval, 0);
        switch (ret) {
        case KindOfNull:   return false;
        case KindOfInt64:
        case KindOfDouble: return true;
        default:
          ASSERT(false);
          break;
        }
      }
      return false;
    } else {
      break;
    }
  case KindOfString:
    if (checkString) {
      return m_data.pstr->isNumeric();
    } else {
      break;
    }
  case KindOfVariant:
    return m_data.pvar->isNumeric(checkString);
  default:
    break;
  }
  return false;
}

bool Variant::isScalar() const {
  switch (getType()) {
  case KindOfNull:
  case KindOfArray:
  case KindOfObject:
    return false;
  default:
    break;
  }
  return true;
}

bool Variant::isResource() const {
  if (is(KindOfObject)) {
    return toObject()->isResource();
  }
  return false;
}

bool Variant::instanceof(const char *s) const {
  if (m_type == KindOfObject) {
    return toObject().instanceof(s);
  }
  if (m_type == KindOfVariant) {
    return m_data.pvar->instanceof(s);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// array operations

Variant Variant::pop() {
  if (m_type == KindOfVariant) {
    return m_data.pvar->pop();
  }
  if (!is(KindOfArray)) {
    throw_bad_type_exception("expecting an array");
    return null;
  }

  Variant ret;
  ArrayData *newarr = getArrayData()->pop(ret);
  if (newarr) {
    set(newarr);
  }
  return ret;
}

Variant Variant::dequeue() {
  if (m_type == KindOfVariant) {
    return m_data.pvar->dequeue();
  }
  if (!is(KindOfArray)) {
    throw_bad_type_exception("expecting an array");
    return null;
  }

  Variant ret;
  ArrayData *newarr = getArrayData()->dequeue(ret);
  if (newarr) {
    set(newarr);
  }
  return ret;
}

void Variant::insert(int pos, CVarRef v) {
  if (m_type == KindOfVariant) {
    return m_data.pvar->insert(pos, v);
  }
  if (isNull()) {
    set(Array::Create());
  }

  if (is(KindOfArray)) {
    ArrayData *oldArr = getArrayData();
    ArrayData *newarr = oldArr->insert(pos, v, (oldArr->getCount() > 1));
    if (newarr) {
      set(newarr);
    }
  } else {
    throw_bad_type_exception("expecting an array");
  }
}

Variant Variant::array_iter_reset() {
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      arr = arr->copy();
      set(arr);
      ASSERT(arr == getArrayData());
    }
    return arr->reset();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_prev() {
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      arr = arr->copy();
      set(arr);
      ASSERT(arr == getArrayData());
    }
    return arr->prev();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_current() const {
  if (is(KindOfArray)) {
    return getArrayData()->current();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_next() {
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      arr = arr->copy();
      set(arr);
      ASSERT(arr == getArrayData());
    }
    return arr->next();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_end() {
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      arr = arr->copy();
      set(arr);
      ASSERT(arr == getArrayData());
    }
    return arr->end();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_key() const {
  if (is(KindOfArray)) {
    return getArrayData()->key();
  }
  throw_bad_type_exception("expecting an array");
  return false;
}

Variant Variant::array_iter_value(ssize_t &pos) const {
  if (is(KindOfArray)) {
    return getArrayData()->value(pos);
  }
  throw_bad_type_exception("expecting an array");
  return null;
}

Variant Variant::array_iter_each() {
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      arr = arr->copy();
      set(arr);
      ASSERT(arr == getArrayData());
    }
    return arr->each();
  }
  throw_bad_type_exception("expecting an array");
  return null;
}

///////////////////////////////////////////////////////////////////////////////
// unary plus
Variant Variant::operator+() {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble()) {
    set(toDouble());
  } else if (isIntVal()) {
    set(toInt64());
  } else if (isString()) {
    String s = toString();
    DataType ret = KindOfNull;
    int64 lval; double dval;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      set(dval);
    } else if (ret == KindOfInt64) {
      set(lval);
    } else {
      set(toInt64());
    }
  } else {
    ASSERT(false);
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// add or array append

Variant Variant::operator+(CVarRef var) const {
  if (is(KindOfArray) && var.is(KindOfArray)) {
    return toArray() + var.toArray();
  }
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayMergeException();
  }
  if (isDouble() || var.isDouble()) {
    return toDouble() + var.toDouble();
  } else if (isIntVal() && var.isIntVal()) {
    return toInt64() + var.toInt64();
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        return dval + var.toDouble();
      }
    }
    if (var.isString()) {
      String s = var.toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        return toDouble() + dval;
      }
    }
  }
  return toInt64() + var.toInt64();
}

Variant &Variant::operator+=(CVarRef var) {
  if (is(KindOfArray) && var.is(KindOfArray)) {
    set(toArray() + var.toArray());
    return *this;
  }
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayMergeException();
  }
  if (isDouble() || var.isDouble()) {
    set(toDouble() + var.toDouble());
  } else if (isIntVal() && var.isIntVal()) {
    set(toInt64() + var.toInt64());
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval + var.toDouble());
        return *this;
      }
    }
    if (var.isString()) {
      String s = var.toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(toDouble() + dval);
        return *this;
      }
    }
    set(toInt64() + var.toInt64());
  }
  return *this;
}

Variant &Variant::operator+=(int64 n) {
  if (is(KindOfArray)) {
    throw BadArrayMergeException();
  }
  if (isDouble()) {
    set(toDouble() + n);
  } else if (isIntVal()) {
    set(toInt64() + n);
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval + n);
        return *this;
      }
    } else {
      ASSERT(false);
    }
    set(toInt64() + n);
  }
  return *this;
}

Variant &Variant::operator+=(double n) {
  if (is(KindOfArray)) {
    throw BadArrayMergeException();
  }
  set(toDouble() + n);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// minus

Variant Variant::operator-() {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble()) {
    set(-toDouble());
  } else if (isIntVal()) {
    set(-toInt64());
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(-dval);
      } else if (ret == KindOfInt64) {
        set(-lval);
      } else {
        set(-toInt64());
      }
    } else {
      ASSERT(false);
    }
  }
  return *this;
}

Variant Variant::operator-(CVarRef var) const {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble() || var.isDouble()) {
    return toDouble() - var.toDouble();
  }
  if (isIntVal() && var.isIntVal()) {
    return toInt64() - var.toInt64();
  }
  if (isString()) {
    String s = toString();
    DataType ret = KindOfNull;
    int64 lval; double dval;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      return dval - var.toDouble();
    }
  }
  if (var.isString()) {
    String s = var.toString();
    DataType ret = KindOfNull;
    int64 lval; double dval;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      return toDouble() - dval;
    }
  }
  return toInt64() - var.toInt64();
}

Variant &Variant::operator-=(CVarRef var) {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble() || var.isDouble()) {
    set(toDouble() - var.toDouble());
  } else if (isIntVal() && var.isIntVal()) {
    set(toInt64() - var.toInt64());
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval - var.toDouble());
        return *this;
      }
    }
    if (var.isString()) {
      String s = var.toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(toDouble() - dval);
        return *this;
      }
    }
    set(toInt64() - var.toInt64());
  }
  return *this;
}

Variant &Variant::operator-=(int64 n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble()) {
    set(toDouble() - n);
  } else if (isIntVal()) {
    set(toInt64() - n);
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval - n);
        return *this;
      }
    } else {
      ASSERT(false);
    }
    set(toInt64() - n);
  }
  return *this;
}

Variant &Variant::operator-=(double n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  set(toDouble() - n);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// multiply

Variant Variant::operator*(CVarRef var) const {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble() || var.isDouble()) {
    return toDouble() * var.toDouble();
  }
  if (isIntVal() && var.isIntVal()) {
    return toInt64() * var.toInt64();
  }
  if (isString()) {
    String s = toString();
    DataType ret = KindOfNull;
    int64 lval; double dval;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      return dval * var.toDouble();
    }
  }
  if (var.isString()) {
    String s = var.toString();
    DataType ret = KindOfNull;
    int64 lval; double dval;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      return toDouble() * dval;
    }
  }
  return toInt64() * var.toInt64();
}

Variant &Variant::operator*=(CVarRef var) {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble() || var.isDouble()) {
    set(toDouble() * var.toDouble());
  } else if (isIntVal() && var.isIntVal()) {
    set(toInt64() * var.toInt64());
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval * var.toDouble());
        return *this;
      }
    }
    if (var.isString()) {
      String s = var.toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(toDouble() * dval);
        return *this;
      }
    }
    set(toInt64() * var.toInt64());
  }
  return *this;
}

Variant &Variant::operator*=(int64 n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (isDouble()) {
    set(toDouble() * n);
  } else if (isIntVal()) {
    set(toInt64() * n);
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfDouble) {
        set(dval * n);
        return *this;
      }
    } else {
      ASSERT(false);
    }
    set(toInt64() * n);
  }
  return *this;
}

Variant &Variant::operator*=(double n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  set(toDouble() * n);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// divide

Variant Variant::operator/(CVarRef var) const {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  int64 lval; double dval; bool int1 = true;
  int64 lval2; double dval2; bool int2 = true;

  if (isDouble()) {
    dval = toDouble();
    int1 = false;
  } else if (isIntVal()) {
    lval = toInt64();
  } else if (isString()) {
    String s = toString();
    DataType ret = KindOfNull;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      int1 = false;
    } else if (ret != KindOfInt64) {
      lval = 0;
    }
  } else {
    ASSERT(false);
  }
  if (var.isDouble()) {
    dval2 = var.toDouble();
    int2 = false;
  } else if (var.isIntVal()) {
    lval2 = var.toInt64();
  } else if (var.isString()) {
    String s = var.toString();
    DataType ret = KindOfNull;
    ret = is_numeric_string(s.data(), s.size(), &lval2, &dval2, 1);
    if (ret == KindOfDouble) {
      int2 = false;
    } else if (ret != KindOfInt64) {
      lval2 = 0;
    }
  } else {
    ASSERT(false);
  }

  if ((int2 && lval2 == 0) || (!int2 && dval2 == 0)) {
    Logger::Warning("Division by zero");
    return false;
  }

  if (int1 && int2) {
    if (lval % lval2 == 0) {
      return lval / lval2;
    } else {
      return (double)lval / lval2;
    }
  } else if (int1 && !int2) {
    return lval / dval2;
  } else if (!int1 && int2) {
    return dval / lval2;
  } else {
    return dval / dval2;
  }
}

Variant &Variant::operator/=(CVarRef var) {
  if (is(KindOfArray) || var.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  int64 lval; double dval; bool int1 = true;
  int64 lval2; double dval2; bool int2 = true;

  if (isDouble()) {
    dval = toDouble();
    int1 = false;
  } else if (isIntVal()) {
    lval = toInt64();
  } else if (isString()) {
    String s = toString();
    DataType ret = KindOfNull;
    ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
    if (ret == KindOfDouble) {
      int1 = false;
    } else if (ret != KindOfInt64) {
      lval = 0;
    }
  } else {
    ASSERT(false);
  }
  if (var.isDouble()) {
    dval2 = var.toDouble();
    int2 = false;
  } else if (var.isIntVal()) {
    lval2 = var.toInt64();
  } else if (var.isString()) {
    String s = var.toString();
    DataType ret = KindOfNull;
    ret = is_numeric_string(s.data(), s.size(), &lval2, &dval2, 1);
    if (ret == KindOfDouble) {
      int2 = false;
    } else if (ret != KindOfInt64) {
      lval2 = 0;
    }
  } else {
    ASSERT(false);
  }

  if ((int2 && lval2 == 0) || (!int2 && dval2 == 0)) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }

  if (int1 && int2) {
    if (lval % lval2 == 0) {
      set(lval / lval2);
    } else {
      set((double)lval / lval2);
    }
  } else if (int1 && !int2) {
    set(lval / dval2);
  } else if (!int1 && int2) {
    set(dval / lval2);
  } else {
    set(dval / dval2);
  }
  return *this;
}

Variant &Variant::operator/=(int64 n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (n == 0) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }

  if (isIntVal() && toInt64() % n == 0) {
    set(toInt64() / n);
  } else if (isDouble()) {
    set(toDouble() / n);
  } else {
    if (isString()) {
      String s = toString();
      DataType ret = KindOfNull;
      int64 lval; double dval;
      ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
      if (ret == KindOfInt64 && lval % n == 0) {
        set(lval / n);
        return *this;
      }
    }
    set(toDouble() / n);
  }
  return *this;
}

Variant &Variant::operator/=(double n) {
  if (is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  if (n == 0.0) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }
  set(toDouble() / n);
  return *this;
}

double operator/(double n, CVarRef v) {
  if (v.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  double dval = v.toDouble();
  if (dval == 0.0) {
    Logger::Warning("Division by zero");
    return false;
  }
  return n / dval;
}

double operator/(int n, CVarRef v) {
  if (v.is(KindOfArray)) {
    throw BadArrayOperandException();
  }
  double dval = v.toDouble();
  if (dval == 0.0) {
    Logger::Warning("Division by zero");
    return false;
  }
  return n / dval;
}

///////////////////////////////////////////////////////////////////////////////
// modulus

int64 Variant::operator%(CVarRef var) const {
  int64 lval = toInt64();
  int64 lval2 = var.toInt64();
  if (lval2 == 0) {
    Logger::Warning("Division by zero");
    return false;
  }
  return lval % lval2;
}

Variant &Variant::operator%=(CVarRef var) {
  int64 lval = toInt64();
  int64 lval2 = var.toInt64();
  if (lval2 == 0) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }
  set(lval % lval2);
  return *this;
}

Variant &Variant::operator%=(int64 n) {
  if (n == 0) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }
  set(toInt64() % n);
  return *this;
}

Variant &Variant::operator%=(double n) {
  if ((int64)n == 0.0) {
    Logger::Warning("Division by zero");
    set(false);
    return *this;
  }
  set(toInt64() % (int64)n);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// bitwise

Variant Variant::operator~() const {
  switch (getType()) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    return ~toInt64();
  case KindOfDouble:
    return ~(int64)(toDouble());
  case LiteralString:
  case KindOfString:
    return ~toString();
  default:
    break;
  }
  throw InvalidOperandException("only numerics and strings can be negated");
}

Variant Variant::operator|(CVarRef v) const {
  if (isString() && v.isString()) {
    return toString() | v.toString();
  }
  return toInt64() | v.toInt64();
}

Variant Variant::operator&(CVarRef v) const {
  if (isString() && v.isString()) {
    return toString() & v.toString();
  }
  return toInt64() & v.toInt64();
}

Variant Variant::operator^(CVarRef v) const {
  if (isString() && v.isString()) {
    return toString() ^ v.toString();
  }
  return toInt64() ^ v.toInt64();
}

Variant &Variant::operator|=(CVarRef v) {
  if (isString() && v.isString()) {
    set(toString() | v.toString());
  } else {
    set(toInt64() | v.toInt64());
  }
  return *this;
}

Variant &Variant::operator&=(CVarRef v) {
  if (isString() && v.isString()) {
    set(toString() & v.toString());
  } else {
    set(toInt64() & v.toInt64());
  }
  return *this;
}

Variant &Variant::operator^=(CVarRef v) {
  if (isString() && v.isString()) {
    set(toString() ^ v.toString());
  } else {
    set(toInt64() ^ v.toInt64());
  }
  return *this;
}

Variant &Variant::operator<<=(int64 n) {
  set(toInt64() << n);
  return *this;
}

Variant &Variant::operator>>=(int64 n) {
  set(toInt64() >> n);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// increment/decrement

Variant &Variant::operator++() {
  switch (getType()) {
  case KindOfNull:   set(1LL); break;
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:  set(toInt64() + 1);  break;
  case KindOfDouble: set(toDouble() + 1); break;
  case LiteralString:
  case KindOfString:
    {
      String s = toString();
      if (s.empty()) {
        set(1LL);
      } else {
        int64 lval; double dval;
        DataType ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
        switch (ret) {
        case KindOfInt64:  set(lval + 1); break;
        case KindOfDouble: set(dval + 1); break;
        case KindOfNull:
          split();
          if (getType() == LiteralString) {
            escalateString();
          }
          getStringData()->inc(); break;
        default:
          ASSERT(false);
          break;
        }
      }
    }
    break;
  default:
    break;
  }
  return *this;
}

Variant Variant::operator++(int) {
  Variant ret(*this);
  operator++();
  return ret;
}

Variant &Variant::operator--() {
  switch (getType()) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:  set(toInt64() - 1);  break;
  case KindOfDouble: set(toDouble() - 1); break;
  case LiteralString:
  case KindOfString:
    {
      String s = toString();
      if (s.empty()) {
        set(-1LL);
      } else {
        int64 lval; double dval;
        DataType ret = is_numeric_string(s.data(), s.size(), &lval, &dval, 1);
        switch (ret) {
        case KindOfInt64:  set(lval - 1);   break;
        case KindOfDouble: set(dval - 1);   break;
        case KindOfNull:   /* do nothing */ break;
        default:
          ASSERT(false);
          break;
        }
      }
    }
    break;
  default:
    break;
  }
  return *this;
}

Variant Variant::operator--(int) {
  Variant ret(*this);
  operator--();
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// iterator functions

ArrayIterPtr Variant::begin(const char *context /* = NULL */) const {
  if (is(KindOfArray)) {
    return new ArrayIter(getArrayData());
  }
  if (is(KindOfObject)) {
    ObjectData *obj = getObjectData();
    if (obj->o_instanceof("iterator")) {
      return new ObjectArrayIter(obj);
    } else {
      return new ArrayIter(obj->o_toIterArray(context));
    }
  }
  return new ArrayIter(NULL);
}

MutableArrayIterPtr Variant::begin(Variant *key, Variant &val) {
  if (is(KindOfObject)) {
    ObjectData *obj = getObjectData();
    if (obj->o_instanceof("iterator")) {
      throw FatalErrorException("An iterator cannot be used with "
                                "foreach by reference");
    }
  }
  // we are about to modify an array that has other weak references, so
  // we have to make a copy to preserve other instances
  if (is(KindOfArray)) {
    ArrayData *arr = getArrayData();
    if (arr->getCount() > 1) {
      set(arr->copy());
    }
  }
  return new MutableArrayIter(this, key, val);
}

void Variant::escalate() {
  if (is(KindOfArray)) {
    Array arr = toArray();
    arr.escalate();
    set(arr);
  }
}

///////////////////////////////////////////////////////////////////////////////
// type conversions

bool Variant::toBoolean() const {
  switch (m_type) {
  case KindOfNull:    return false;
  case KindOfDouble:  return m_data.dbl != 0;
  case LiteralString: return StringData(m_data.str).toBoolean();
  case KindOfString:  return m_data.pstr->toBoolean();
  case KindOfArray:   return !m_data.parr->empty();
  case KindOfObject:  return m_data.pobj != NULL;
  case KindOfVariant: return m_data.pvar->toBoolean();
  default:
    break;
  }
  return m_data.num != 0;
}

int64 Variant::toInt64(int base /* = 10 */) const {
  switch (m_type) {
  case KindOfNull:    return 0;
  case KindOfDouble:  {
    return (m_data.dbl > LONG_MAX) ? (uint64)m_data.dbl : (int64)m_data.dbl;
  }
  case LiteralString: return StringData(m_data.str).toInt64(base);
  case KindOfString:  return m_data.pstr->toInt64(base);
  case KindOfArray:   return m_data.parr->empty() ? 0 : 1;
  case KindOfObject:  return m_data.pobj ? m_data.pobj->o_toInt64() : 0;
  case KindOfVariant: return m_data.pvar->toInt64(base);
  default:
    break;
  }
  return m_data.num;
}

double Variant::toDouble() const {
  switch (m_type) {
  case KindOfNull:    return 0.0;
  case KindOfDouble:  return m_data.dbl;
  case LiteralString: return StringData(m_data.str).toDouble();
  case KindOfString:  return m_data.pstr->toDouble();
  case KindOfVariant: return m_data.pvar->toDouble();
  default:
    break;
  }
  return (double)toInt64();
}

String Variant::toString() const {
  switch (m_type) {
  case KindOfNull:    return "";
  case KindOfBoolean: return m_data.num ? "1" : "";
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:   return m_data.num;
  case KindOfDouble:  return m_data.dbl;
  case LiteralString: return m_data.str;
  case KindOfString:  return m_data.pstr;
  case KindOfArray:   return "Array";
  case KindOfObject:  return m_data.pobj->t___tostring();
  case KindOfVariant: return m_data.pvar->toString();
  default:
    ASSERT(false);
    break;
  }
  return "";
}

Array Variant::toArray() const {
  switch (m_type) {
  case KindOfNull:    return Array::Create();
  case KindOfInt64:   return Array::Create(m_data.num);
  case LiteralString: return Array::Create(m_data.str);
  case KindOfString:  return Array::Create(String(m_data.pstr));
  case KindOfArray:   return m_data.parr;
  case KindOfObject:  return m_data.pobj->o_toArray();
  case KindOfVariant: return m_data.pvar->toArray();
  default:
    break;
  }
  return Array::Create(*this);
}

Object Variant::toObject() const {
  if (m_type == KindOfObject) return m_data.pobj;
  if (m_type == KindOfVariant) return m_data.pvar->toObject();

  switch (m_type) {
  case KindOfNull:
    break;
  case KindOfBoolean:
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
  case KindOfDouble:
  case LiteralString:
  case KindOfString:
    {
      c_stdclass *obj = NEW(c_stdclass)();
      obj->o_lval("scalar", -1) = *this;
      return obj;
    }
  case KindOfArray:   return m_data.parr->toObject();
  default:
    ASSERT(false);
    break;
  }
  return Object(NEW(c_stdclass)());
}

Variant Variant::toKey() const {
  switch (m_type) {
  case KindOfNull:
    return String("");
  case LiteralString:
    return String(m_data.str).toKey();
  case KindOfString:
    {
      int64 n;
      if (m_data.pstr->isStrictlyInteger(n)) {
        return Variant(n);
      } else {
        return Variant(*this);
      }
    }
  case KindOfBoolean:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    return toInt64();
  case KindOfDouble:
    return (int64)m_data.dbl;
  case KindOfObject:
    if (isResource()) {
      return toInt64();
    }
  case KindOfVariant:
    return m_data.pvar->toKey();
  default:
    break;
  }
  throw_bad_type_exception("Invalid type used as key");
  return null;
}

Variant::operator String() const {
  return toString();
}

Variant::operator Array() const {
  return toArray();
}

Variant::operator Object() const {
  return toObject();
}

///////////////////////////////////////////////////////////////////////////////
// comparisons

bool Variant::same(bool v2) const {
  return is(KindOfBoolean) && equal(v2);
}

bool Variant::same(char v2) const {
  return same((int64)v2);
}

bool Variant::same(short v2) const {
  return same((int64)v2);
}

bool Variant::same(int v2) const {
  return same((int64)v2);
}

bool Variant::same(int64 v2) const {
  switch (getType()) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    return equal(v2);
  default:
    break;
  }
  return false;
}

bool Variant::same(double v2) const {
  return isDouble() && equal(v2);
}

bool Variant::same(litstr v2) const {
  return same(String(v2));
}

bool Variant::same(CStrRef v2) const {
  bool null1 = isNull();
  bool null2 = v2.isNull();
  if (null1 && null2) return true;
  if (null1 || null2) return false;
  return isString() && equal(v2);
}

bool Variant::same(CArrRef v2) const {
  bool null1 = isNull();
  bool null2 = v2.isNull();
  if (null1 && null2) return true;
  if (null1 || null2) return false;
  return is(KindOfArray) && toArray().same(v2);
}

bool Variant::same(CObjRef v2) const {
  bool null1 = isNull();
  bool null2 = v2.isNull();
  if (null1 && null2) return true;
  if (null1 || null2) return false;
  return is(KindOfObject) && getObjectData() == v2.get();
}

bool Variant::same(CVarRef v2) const {
  bool null1 = isNull();
  bool null2 = v2.isNull();
  if (null1 && null2) return true;
  if (null1 || null2) return false;

  switch (getType()) {
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    switch (v2.getType()) {
    case KindOfByte:
    case KindOfInt16:
    case KindOfInt32:
    case KindOfInt64:
      return equal(v2);
    default:
      break;
    }
    break;
  case LiteralString:
  case KindOfString:
    return v2.isString() && equal(v2);
  case KindOfArray:
    if (v2.is(KindOfArray)) {
      return toArray().same(v2.toArray());
    }
    break;
  case KindOfObject:
    return v2.is(KindOfObject) && getObjectData() == v2.getObjectData();
  default:
    break;
  }
  return getType() == v2.getType() && equal(v2);
}

///////////////////////////////////////////////////////////////////////////////

#define UNWRAP(reverse)                                         \
  switch (getType()) {                                          \
  case KindOfNull:                                              \
  case KindOfBoolean: return HPHP::reverse(v2, toBoolean());    \
  case KindOfByte:                                              \
  case KindOfInt16:                                             \
  case KindOfInt32:                                             \
  case KindOfInt64:   return HPHP::reverse(v2, toInt64());      \
  case KindOfDouble:  return HPHP::reverse(v2, toDouble());     \
  case LiteralString:                                           \
  case KindOfString:  return HPHP::reverse(v2, toString());     \
  case KindOfArray:   return HPHP::reverse(v2, toArray());      \
  case KindOfObject:  return HPHP::reverse(v2, toObject());     \
  default:                                                      \
    ASSERT(false);                                              \
    break;                                                      \
  }                                                             \
  return false;                                                 \

// "null" needs to convert to "" before comparing with a string
#define UNWRAP_STR(reverse)                                     \
  switch (getType()) {                                          \
  case KindOfNull:    return HPHP::reverse(v2, "");             \
  case KindOfBoolean: return HPHP::reverse(v2, toBoolean());    \
  case KindOfByte:                                              \
  case KindOfInt16:                                             \
  case KindOfInt32:                                             \
  case KindOfInt64:   return HPHP::reverse(v2, toInt64());      \
  case KindOfDouble:  return HPHP::reverse(v2, toDouble());     \
  case LiteralString:                                           \
  case KindOfString:  return HPHP::reverse(v2, toString());     \
  case KindOfArray:   return HPHP::reverse(v2, toArray());      \
  case KindOfObject:  return HPHP::reverse(v2, toObject());     \
  default:                                                      \
    ASSERT(false);                                              \
    break;                                                      \
  }                                                             \
  return false;                                                 \

// "null" needs to convert to "" before comparing with a string
#define UNWRAP_VAR(forward, reverse)                            \
  switch (getType()) {                                          \
  case KindOfNull:                                              \
    if (v2.isString()) {                                        \
      return HPHP::reverse(v2, "");                             \
    } /* otherwise fall through */                              \
  case KindOfBoolean: return HPHP::reverse(v2, toBoolean());    \
  case KindOfByte:                                              \
  case KindOfInt16:                                             \
  case KindOfInt32:                                             \
  case KindOfInt64:   return HPHP::reverse(v2, toInt64());      \
  case KindOfDouble:  return HPHP::reverse(v2, toDouble());     \
  case LiteralString:                                           \
  case KindOfString:  return HPHP::reverse(v2, toString());     \
  case KindOfArray:                                             \
    if (v2.is(KindOfArray)) {                                   \
      return toArray().forward(v2.toArray());                   \
    }                                                           \
    return HPHP::reverse(v2, toArray());                        \
  case KindOfObject:  return HPHP::reverse(v2, toObject());     \
  default:                                                      \
    ASSERT(false);                                              \
    break;                                                      \
  }                                                             \
  return false;                                                 \

// array comparison is directional when they are uncomparable
// also, ">" is implemented as "!<=" in Zend
#define UNWRAP_ARR(forward, reverse)                            \
  switch (getType()) {                                          \
  case KindOfNull:                                              \
  case KindOfBoolean: return HPHP::reverse(v2, toBoolean());    \
  case KindOfByte:                                              \
  case KindOfInt16:                                             \
  case KindOfInt32:                                             \
  case KindOfInt64:   return HPHP::reverse(v2, toInt64());      \
  case KindOfDouble:  return HPHP::reverse(v2, toDouble());     \
  case LiteralString:                                           \
  case KindOfString:  return HPHP::reverse(v2, toString());     \
  case KindOfArray:   return toArray().forward(v2);             \
  case KindOfObject:  return HPHP::reverse(v2, toObject());     \
  default:                                                      \
    ASSERT(false);                                              \
    break;                                                      \
  }                                                             \
  return false;                                                 \

bool Variant::equal(bool    v2) const { UNWRAP(equal);}
bool Variant::equal(char    v2) const { UNWRAP(equal);}
bool Variant::equal(short   v2) const { UNWRAP(equal);}
bool Variant::equal(int     v2) const { UNWRAP(equal);}
bool Variant::equal(int64   v2) const { UNWRAP(equal);}
bool Variant::equal(double  v2) const { UNWRAP(equal);}
bool Variant::equal(litstr  v2) const { UNWRAP_STR(equal);}
bool Variant::equal(CStrRef v2) const { UNWRAP_STR(equal);}
bool Variant::equal(CArrRef v2) const { UNWRAP(equal);}
bool Variant::equal(CObjRef v2) const { UNWRAP(equal);}
bool Variant::equal(CVarRef v2) const { UNWRAP_VAR(equal,equal);}

bool Variant::less(bool    v2) const { UNWRAP(more);}
bool Variant::less(char    v2) const { UNWRAP(more);}
bool Variant::less(short   v2) const { UNWRAP(more);}
bool Variant::less(int     v2) const { UNWRAP(more);}
bool Variant::less(int64   v2) const { UNWRAP(more);}
bool Variant::less(double  v2) const { UNWRAP(more);}
bool Variant::less(litstr  v2) const { UNWRAP_STR(more);}
bool Variant::less(CStrRef v2) const { UNWRAP_STR(more);}
bool Variant::less(CArrRef v2) const { UNWRAP_ARR(less,more);}
bool Variant::less(CObjRef v2) const { UNWRAP(more);}
bool Variant::less(CVarRef v2) const { UNWRAP_VAR(less,more);}

bool Variant::more(bool    v2) const { UNWRAP(less);}
bool Variant::more(char    v2) const { UNWRAP(less);}
bool Variant::more(short   v2) const { UNWRAP(less);}
bool Variant::more(int     v2) const { UNWRAP(less);}
bool Variant::more(int64   v2) const { UNWRAP(less);}
bool Variant::more(double  v2) const { UNWRAP(less);}
bool Variant::more(litstr  v2) const { UNWRAP_STR(less);}
bool Variant::more(CStrRef v2) const { UNWRAP_STR(less);}
bool Variant::more(CArrRef v2) const { UNWRAP_ARR(more,less);}
bool Variant::more(CObjRef v2) const { UNWRAP(less);}
bool Variant::more(CVarRef v2) const { UNWRAP_VAR(more,less);}

///////////////////////////////////////////////////////////////////////////////
// comparison operators

bool Variant::operator==(CVarRef v) const {
  return HPHP::equal(*this, v);
}

bool Variant::operator!=(CVarRef v) const {
  return !HPHP::equal(*this, v);
}

bool Variant::operator>=(CVarRef v) const {
  return not_less(*this, v);
}

bool Variant::operator<=(CVarRef v) const {
  return not_more(*this, v);
}

bool Variant::operator>(CVarRef v) const {
  return HPHP::more(*this, v);
}

bool Variant::operator<(CVarRef v) const {
  return HPHP::less(*this, v);
}

///////////////////////////////////////////////////////////////////////////////
// offset functions

ObjectData *Variant::getArrayAccess() const {
  ASSERT(is(KindOfObject));
  ObjectData *obj = getObjectData();
  ASSERT(obj);
  if (!obj->o_instanceof("arrayaccess")) {
    throw InvalidOperandException("not ArrayAccess objects");
  }
  return obj;
}

void Variant::callOffsetUnset(CVarRef key) {
  getArrayAccess()->o_invoke("offsetunset", Array::Create(key), -1);
}

Variant Variant::rvalAt(int64 offset, int64 prehash /* = -1 */) const {
  switch (getType()) {
  case LiteralString:
    escalateString();
    // fall through
  case KindOfString:
    return getStringData()->getChar((int)offset);
  case KindOfArray:
    return getArrayData()->get(offset, prehash);
  case KindOfObject:
    return getArrayAccess()->o_invoke("offsetget", Array::Create(offset), -1);
  default:
    break;
  }
  return null;
}

Variant Variant::rvalAt(litstr offset, int64 prehash /* = -1 */) const {
  switch (getType()) {
  case LiteralString:
    escalateString();
    // fall through
  case KindOfString:
    return getStringData()->getChar(String(offset).toInt32());
  case KindOfArray:
    return getArrayData()->get(String(offset).toKey(), prehash);
  case KindOfObject:
    return getArrayAccess()->o_invoke("offsetget", Array::Create(offset), -1);
  default:
    break;
  }
  return null;
}

Variant Variant::rvalAt(CStrRef offset, int64 prehash /* = -1 */) const {
  switch (getType()) {
  case LiteralString:
    escalateString();
    // fall through
  case KindOfString:
    return getStringData()->getChar(offset.toInt32());
  case KindOfArray:
    return getArrayData()->get(offset.toKey(), prehash);
  case KindOfObject:
    return getArrayAccess()->o_invoke("offsetget", Array::Create(offset), -1);
  default:
    break;
  }
  return null;
}

Variant Variant::rvalAt(CVarRef offset, int64 prehash /* = -1 */) const {
  switch (getType()) {
  case LiteralString:
    escalateString();
    // fall through
  case KindOfString:
    return getStringData()->getChar(offset.toInt32());
  case KindOfArray:
    {
      ArrayData* arr = getArrayData();
      switch (offset.getType()) {
      case KindOfBoolean:
      case KindOfByte:
      case KindOfInt16:
      case KindOfInt32:
      case KindOfInt64:
        return arr->get(offset.toInt64(), prehash);
      default:
        {
          Variant k = offset.toKey();
          if (!k.isNull()) {
            return arr->get(k, prehash);
          }
          return null;
        }
      }
    }
  case KindOfObject:
    return getArrayAccess()->o_invoke("offsetget", Array::Create(offset), -1);
  default:
    break;
  }
  return null;
}

Variant &Variant::lvalAt(bool    key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key, prehash);
}
Variant &Variant::lvalAt(char    key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key, prehash);
}
Variant &Variant::lvalAt(short   key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key, prehash);
}
Variant &Variant::lvalAt(int     key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key, prehash);
}
Variant &Variant::lvalAt(int64   key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key, prehash);
}
Variant &Variant::lvalAt(double  key, int64 prehash /* = -1 */) {
  return lvalAtImpl((int64)key, prehash);
}
Variant &Variant::lvalAt(litstr  key, int64 prehash /* = -1 */) {
  return lvalAtImpl(String(key).toKey(), prehash);
}
Variant &Variant::lvalAt(CStrRef key, int64 prehash /* = -1 */) {
  return lvalAtImpl(key.toKey(), prehash);
}
Variant &Variant::lvalAt(CVarRef key, int64 prehash /* = -1 */) {
  Variant k = key.toKey();
  if (!k.isNull()) {
    return lvalAtImpl(k, prehash);
  }
  return lvalBlackHole();
}

Variant &Variant::lvalInvalid() {
  throw_bad_type_exception("not array objects");
  return ((Globals*)get_global_variables())->__lvalProxy;
}

Variant &Variant::lvalBlackHole() {
  return ((Globals*)get_global_variables())->__lvalProxy;
}

Variant Variant::refvalAt(bool    key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(char    key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(short   key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(int     key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(int64   key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(double  key, int64 prehash /* = -1 */) {
  return refvalAtImpl((int64)key, prehash);
}
Variant Variant::refvalAt(litstr  key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(CStrRef key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}
Variant Variant::refvalAt(CVarRef key, int64 prehash /* = -1 */) {
  return refvalAtImpl(key, prehash);
}

Variant Variant::o_get(CStrRef propName, int64 prehash /* = -1 */) const {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_get(propName, prehash);
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_get(propName, prehash);
  } else {
    return null;
  }
}

Variant Variant::o_invoke(const char *s, CArrRef params, int64 hash) {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_invoke(s, params, hash);
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_invoke(s, params, hash);
  } else {
    throw InvalidOperandException(
        "Call to a member function on a non-object");
  }
}

Variant Variant::o_root_invoke(const char *s, CArrRef params, int64 hash) {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_root_invoke(s, params, hash);
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_root_invoke(s, params, hash);
  } else {
    throw InvalidOperandException(
        "Call to a member function on a non-object");
  }
}

Variant Variant::o_invoke_ex(const char *clsname, const char *s,
                              CArrRef params, int64 hash) {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_invoke_ex(clsname, s, params, hash);
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_invoke_ex(clsname, s, params, hash);
  } else {
    throw InvalidOperandException(
        "Call to a member function on a non-object");
  }
}

Variant Variant::o_invoke_few_args(const char *s, int64 hash, int count,
                                   CVarRef a0 /* = null_variant */,
                                   CVarRef a1 /* = null_variant */,
                                   CVarRef a2 /* = null_variant */
#if INVOKE_FEW_ARGS_COUNT > 3
                                   ,CVarRef a3 /* = null_variant */,
                                   CVarRef a4 /* = null_variant */,
                                   CVarRef a5 /* = null_variant */
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                   ,CVarRef a6 /* = null_variant */,
                                   CVarRef a7 /* = null_variant */,
                                   CVarRef a8 /* = null_variant */,
                                   CVarRef a9 /* = null_variant */
#endif
) {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_invoke_few_args(s, hash, count, a0, a1, a2
#if INVOKE_FEW_ARGS_COUNT > 3
                                          ,a3, a4, a5
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                          ,a6, a7, a8, a9
#endif
                                         );
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_invoke_few_args(s, hash, count, a0, a1, a2
#if INVOKE_FEW_ARGS_COUNT > 3
                                          ,a3, a4, a5
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                          ,a6, a7, a8, a9
#endif
                                         );
  } else {
    throw InvalidOperandException(
        "Call to a member function on a non-object");
  }
}

Variant Variant::o_root_invoke_few_args(const char *s, int64 hash, int count,
                                        CVarRef a0 /* = null_variant */,
                                        CVarRef a1 /* = null_variant */,
                                        CVarRef a2 /* = null_variant */
#if INVOKE_FEW_ARGS_COUNT > 3
                                        ,CVarRef a3 /* = null_variant */,
                                        CVarRef a4 /* = null_variant */,
                                        CVarRef a5 /* = null_variant */
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                        ,CVarRef a6 /* = null_variant */,
                                        CVarRef a7 /* = null_variant */,
                                        CVarRef a8 /* = null_variant */,
                                        CVarRef a9 /* = null_variant */
#endif
) {
  if (m_type == KindOfObject) {
    return m_data.pobj->o_root_invoke_few_args(s, hash, count, a0, a1, a2
#if INVOKE_FEW_ARGS_COUNT > 3
                                               ,a3, a4, a5
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                               ,a6, a7, a8, a9
#endif
                                              );
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_root_invoke_few_args(s, hash, count, a0, a1, a2
#if INVOKE_FEW_ARGS_COUNT > 3
                                               ,a3, a4, a5
#endif
#if INVOKE_FEW_ARGS_COUNT > 6
                                               ,a6, a7, a8, a9
#endif
                                              );
  } else {
    throw InvalidOperandException(
        "Call to a member function on a non-object");
  }
}

ObjectOffset Variant::o_lval(CStrRef propName, int64 prehash /*= -1 */) {
  if (m_type == KindOfObject) {
    return Object(m_data.pobj).o_lval(propName, prehash);
  } else if (m_type == KindOfVariant) {
    return m_data.pvar->o_lval(propName, prehash);
  } else {
    set(Object(NEW(c_stdclass)()));
    return Object(m_data.pobj).o_lval(propName, prehash);
  }
}

#define IMPLEMENT_SETAT(T)                                              \
  CVarRef Variant::setAtImpl(T key, CVarRef v, int64 prehash) {         \
    switch (m_type) {                                                   \
    case KindOfBoolean:                                                 \
      if (toBoolean()) {                                                \
        throw_bad_type_exception("not array objects");                  \
        break;                                                          \
      }                                                                 \
      /* Fall through */                                                \
    case KindOfNull:                                                    \
      set(ArrayData::Create(key, v));                                   \
      break;                                                            \
    case KindOfArray:                                                   \
      {                                                                 \
        if (v.isContagious()) {                                         \
          escalate();                                                   \
        }                                                               \
        ArrayData *escalated =                                          \
          m_data.parr->set(key, v, (m_data.parr->getCount() > 1),       \
                           prehash);                                    \
        if (escalated) {                                                \
          set(escalated);                                               \
        }                                                               \
      }                                                                 \
      break;                                                            \
    case KindOfVariant:                                                 \
      m_data.pvar->set(key, v);                                         \
      break;                                                            \
    case LiteralString:                                                 \
    case KindOfString:                                                  \
      {                                                                 \
        String s = toString();                                          \
        if (s.empty()) {                                                \
          set(Array::Create(key, v));                                   \
        } else {                                                        \
          s.lvalAt(key) = v;                                            \
          set(s);                                                       \
        }                                                               \
        break;                                                          \
      }                                                                 \
    case KindOfObject:                                                  \
      getArrayAccess()->o_invoke("offsetset",                           \
                                 CREATE_VECTOR2(key, v), -1);           \
      break;                                                            \
    default:                                                            \
      throw_bad_type_exception("not array objects");                    \
      break;                                                            \
    }                                                                   \
    return v;                                                           \
  }                                                                     \

IMPLEMENT_SETAT(int64);
IMPLEMENT_SETAT(CStrRef);
IMPLEMENT_SETAT(CVarRef);

CVarRef Variant::set(int64 key, CVarRef v, int64 prehash /* = -1 */) {
  return setAtImpl(key, v, prehash);
}

CVarRef Variant::set(litstr key, CVarRef v, int64 prehash /* = -1 */) {
  return setAtImpl(String(key).toKey(), v, prehash);
}

CVarRef Variant::set(CStrRef key, CVarRef v, int64 prehash /* = -1 */) {
  return setAtImpl(key.toKey(), v, prehash);
}

CVarRef Variant::set(CVarRef key, CVarRef v, int64 prehash /* = -1 */) {
  switch(key.getType()) {
  case KindOfBoolean:
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    return setAtImpl(key.toInt64(), v, prehash);
  default:
    break;
  }
  // Trouble cases: Object and Array
  Variant k = key.toKey();
  if (!k.isNull()) {
    return setAtImpl(k, v, prehash);
  }
  return null_variant;
}

CVarRef Variant::append(CVarRef v) {
  switch (m_type) {
  case KindOfNull:
    set(ArrayData::Create(v));
    break;
  case KindOfArray:
    {
      bool contagious = false;
      if (v.isContagious()) {
        escalate();
        contagious = true;
      }
      ArrayData *escalated =
        m_data.parr->append(v, (m_data.parr->getCount() > 1));
      if (escalated) {
        set(escalated);
        // special case "$a[] = $a;" to match PHP's weird semantics
        if (!contagious &&
            (this == &v ||
             (v.is(KindOfArray) && getArrayData() == v.getArrayData()))) {
          const_cast<Variant&>(escalated->endRef()).set(escalated);
        }
      }
    }
    break;
  case KindOfVariant:
    m_data.pvar->append(v);
    break;
  case KindOfObject:
    {
      Array params = CREATE_VECTOR2(null, v);
      m_data.pobj->o_invoke("offsetset", params, -1);
    }
    break;
  case LiteralString:
  case KindOfString:
    if (toString().empty()) {
      set(ArrayData::Create(v));
      return v;
    }
    // fall through to throw
  default:
    throw_bad_type_exception("[] operator not supported for this type");
  }
  return v;
}

void Variant::remove(litstr  key, int64 prehash /* = -1 */) {
  removeImpl(String(key).toKey(), prehash);
}
void Variant::remove(CStrRef key, int64 prehash /* = -1 */) {
  removeImpl(key.toKey(), prehash);
}

void Variant::remove(CVarRef key, int64 prehash /* = -1 */) {
  switch(key.getType()) {
  case KindOfBoolean:
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    removeImpl(key.toInt64(), prehash);
    return;
  default:
    break;
  }
  // Trouble cases: Array, Object
  Variant k = key.toKey();
  if (!k.isNull()) {
    removeImpl(k, prehash);
  }
}

void Variant::setStatic() const {
  switch (m_type) {
  case KindOfString:
    m_data.pstr->setStatic();
    break;
  case KindOfArray:
    m_data.parr->setStatic();
    m_data.parr->onSetStatic();
    break;
  case KindOfVariant:
    m_data.pvar->setStatic();
    break;
  case KindOfObject:
    ASSERT(false); // object shouldn't be in a scalar array
    break;
  default:
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// output functions

void Variant::serialize(VariableSerializer *serializer,
                        bool isArrayKey /* = false */) const {
  if (m_type == KindOfVariant) {
    // Ugly, but behavior is different for serialize
    if (serializer->getType() == VariableSerializer::Serialize) {
      if (serializer->incNestedLevel(m_data.pvar)) {
        serializer->writeOverflow(m_data.pvar);
      } else {
        m_data.pvar->serialize(serializer, isArrayKey);
      }
      serializer->decNestedLevel(m_data.pvar);
    } else {
      m_data.pvar->serialize(serializer, isArrayKey);
    }
    return;
  }

  switch (m_type) {
  case KindOfNull:
    ASSERT(!isArrayKey);
    serializer->writeNull();                break;
  case KindOfBoolean:
    ASSERT(!isArrayKey);
    serializer->write(m_data.num != 0);     break;
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:
    serializer->write(m_data.num);          break;
  case KindOfDouble:
    serializer->write(m_data.dbl);          break;
  case LiteralString:
    serializer->write(m_data.str);          break;
  case KindOfString:
    serializer->write(m_data.pstr->data(),
                      m_data.pstr->size(), isArrayKey);
    break;
  case KindOfArray:
    ASSERT(!isArrayKey);
    m_data.parr->serialize(serializer);     break;
  case KindOfObject:
    ASSERT(!isArrayKey);
    m_data.pobj->serialize(serializer);     break;
  default:
    ASSERT(false);
    break;
  }
}

void Variant::unserialize(VariableUnserializer *unserializer) {
  std::istream &in = unserializer->in();
  char type, sep;
  in >> type >> sep;

  if (type != 'R') {
    unserializer->add(this);
  }

  if (type == 'N') {
    // ASSERT(isNull());
    if(sep != ';') throw Exception("Expected ';' but got '%c'", sep);
    return;
  }
  if (sep != ':') {
    throw Exception("Expected ':' but got '%c'", sep);
  }

  switch (type) {
  case 'r': { int64 id; in >> id; operator=(unserializer->get(id)); } break;
  case 'R':
    {
      int64 id;
      in >> id;
      operator=(ref(unserializer->get(id)));
    }
    break;
  case 'b': { int64 v;  in >> v; operator=((bool)v); } break;
  case 'i': { int64 v;  in >> v; operator=(v);       } break;
  case 'd':
    {
      double v;
      char ch = in.peek();
      bool negative = false;
      char buf[4];
      if (ch == '-') {
        negative = true;
        in >> ch;
        ch = in.peek();
      }
      if (ch == 'I') {
        in.read(buf, 3); buf[3] = '\0';
        if (strcmp(buf, "INF")) {
          throw Exception("Expected 'INF' but got '%s'", buf);
        }
        v = atof("inf");
      } else if (ch == 'N') {
        in.read(buf, 3); buf[3] = '\0';
        if (strcmp(buf, "NAN")) {
          throw Exception("Expected 'NAN' but got '%s'", buf);
        }
        v = atof("nan");
      } else {
        in >> v;
      }
      operator=(negative ? -v : v);
    }
    break;
  case 's':
    {
      String v;
      v.unserialize(in);
      operator=(v);
    }
    break;
  case 'a':
    {
      Array v = Array::Create();
      v.unserialize(unserializer);
      operator=(v);
      return; // array has '}' terminating
    }
    break;
  case 'O':
    {
      String clsName;
      clsName.unserialize(in);

      in >> sep;
      if (sep != ':') {
        throw Exception("Expected ':' but got '%c'", sep);
      }

      Object obj;
      try {
        obj = create_object(clsName.data(), Array::Create(), false);
      } catch (ClassNotFoundException &e) {
        obj = create_object("__PHP_Incomplete_Class", Array::Create(), false);
        obj->o_set("__PHP_Incomplete_Class_Name", -1, clsName);
      }
      operator=(obj);
      int64 size;
      char sep;
      in >> size >> sep;
      if (sep != ':') {
        throw Exception("Expected ':' but got '%c'", sep);
      }
      in >> sep;
      if (sep != '{') {
        throw Exception("Expected '{' but got '%c'", sep);
      }
      if (size > 0) {
        for (int64 i = 0; i < size; i++) {
          String key = unserializer->unserializeKey().toString();
          int subLen = 0;
          if (key.charAt(0) == '\00') {
            if (key.charAt(1) == '*') {
              subLen = 3; // protected
            } else {
              subLen = key.find('\00', 1) + 1; // private, skipping class name
              if (subLen == String::npos) {
                throw Exception("Mangled private object property");
              }
            }
          }
          Variant &value = subLen != 0 ?
            obj.o_lval(key.substr(subLen), -1).lval() :
            obj.o_lval(key, -1).lval();
          value.unserialize(unserializer);
        }
      }
      in >> sep;
      if (sep != '}') {
        throw Exception("Expected '}' but got '%c'", sep);
      }

      obj->t___wakeup();
      return; // object has '}' terminating
    }
    break;
  default:
    throw Exception("Unknown type '%c'", type);
  }
  in >> sep;
  if (sep != ';') {
    throw Exception("Expected ';' but got '%c'", sep);
  }
}

Variant Variant::share(bool save) const {
  if (m_type == KindOfVariant) {
    return m_data.pvar->share(save);
  }

  switch (m_type) {
  case KindOfNull:    return false; // same as non-existent
  case KindOfBoolean: return (m_data.num != 0);
  case KindOfByte:
  case KindOfInt16:
  case KindOfInt32:
  case KindOfInt64:   return m_data.num;
  case KindOfDouble:  return m_data.dbl;
  case LiteralString: return m_data.str;
  case KindOfString:
    return String(m_data.pstr->data(), m_data.pstr->size(), CopyString);
  case KindOfArray:
    {
      Array ret;
      for (ArrayIter iter(m_data.parr); iter; ++iter) {
        ret.set(iter.first().share(save), iter.second().share(save));
      }
      return ret;
    }
    break;
  case KindOfObject:
    if (save) {
      // we have to return an object so to remember its type
      c_stdclass *obj = NEW(c_stdclass)();
      obj->o_set("s", -1, f_serialize(*this));
      return obj;
    } else {
      return f_unserialize(m_data.pobj->o_get("s", -1));
    }
    break;
  default:
    ASSERT(false);
    break;
  }

  return false; // same as non-existent
}

const char *Variant::getTypeString(DataType type) {
  switch (type) {
  case KindOfNull:    return "KindOfNull";
  case KindOfBoolean: return "KindOfBoolean";
  case KindOfByte:    return "KindOfByte";
  case KindOfInt16:   return "KindOfInt16";
  case KindOfInt32:   return "KindOfInt32";
  case KindOfInt64:   return "KindOfInt64";
  case KindOfDouble:  return "KindOfDouble";
  case LiteralString: return "LiteralString";
  case KindOfString:  return "KindOfString";
  case KindOfArray:   return "KindOfArray";
  case KindOfObject:  return "KindOfObject";
  case KindOfVariant: return "KindOfVariant";
  default:
    ASSERT(false);
    break;
  }
  return "";
}

std::string Variant::getDebugDump() const {
  char buf[1024];
  snprintf(buf, sizeof(buf), "[%s: %p]", getTypeString(m_type), m_data.pstr);
  return buf;
}

void Variant::dump() {
  VariableSerializer vs(VariableSerializer::VarDump);
  Variant ret = vs.serialize(*this, true);
  printf("Variant: %s", ret.toString().data());
}

///////////////////////////////////////////////////////////////////////////////
}
