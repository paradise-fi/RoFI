#include <cmath>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "jsmn.h"
#include "rbjson.h"

#include "mpaland-printf/printf.h"

#define TAG "RbJson"

namespace rbjson {

static int count_tok_size(jsmntok_t* tok) {
    jsmntok_t* itr = tok + 1;
    for (int i = 0; i < tok->size; ++i) {
        itr += count_tok_size(itr);
    }
    return itr - tok;
}

static inline void write_string_escaped(const char* str, std::ostream& ss) {
    const char* start = str;
    const char* end = NULL;
    ss.put('"');
    while (true) {
        end = strchr(start, '"');
        if (end == NULL) {
            ss.write(start, strlen(start));
            ss.put('"');
            return;
        } else {
            ss.write(start, end - start);
            ss.write("\\\"", 2);
            start = end + 1;
        }
    }
}

static void fct_ostream(char c, void* arg) {
    ((std::ostream*)arg)->put(c);
}

static Value* parse_value(char* buf, jsmntok_t* tok);

static Object* parse_object(char* buf, jsmntok_t* obj) {
    if (obj->type != JSMN_OBJECT) {
        return NULL;
    }

    Object* res = new Object();
    jsmntok_t* tok = obj + 1;
    for (int i = 0; i < obj->size; ++i) {
        if (tok->type != JSMN_STRING || tok->size != 1) {
            continue;
        }

        Value* val = parse_value(buf, tok + 1);
        if (val != NULL) {
            std::string key(buf + tok->start, tok->end - tok->start);
            res->set(std::move(key), val);
        }

        tok += count_tok_size(tok);
    }
    return res;
}

static Array* parse_array(char* buf, jsmntok_t* arr) {
    if (arr->type != JSMN_ARRAY) {
        return NULL;
    }

    Array* res = new Array();
    jsmntok_t* tok = arr + 1;
    for (int i = 0; i < arr->size; ++i) {
        Value* val = parse_value(buf, tok);
        if (val != NULL) {
            res->push_back(val);
        }
        tok += count_tok_size(tok);
    }
    return res;
}

Value* parse_value(char* buf, jsmntok_t* tok) {
    switch (tok->type) {
    case JSMN_OBJECT:
        return parse_object(buf, tok);
    case JSMN_ARRAY:
        return parse_array(buf, tok);
    case JSMN_STRING:
        return new String(std::string(buf + tok->start, tok->end - tok->start));
    case JSMN_PRIMITIVE: {
        const char* str = buf + tok->start;
        const int len = tok->end - tok->start;
        if (len == 0) {
            return NULL;
        }

        switch (*str) {
        case 't':
            return new Bool(true);
        case 'f':
            return new Bool(false);
        case 'n':
            return new Nil();
        default: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.*s", len, str);

            char* endptr;
            double val = strtod(buf, &endptr);
            if (buf == endptr) {
                return NULL;
            }
            return new Number(val);
        }
        }
    }
    default:
        return NULL;
    }
}

Object* parse(char* buf, size_t size) {
    jsmn_parser parser;
    size_t tokens_size = 32;
    jsmntok_t tokens_static[32];
    std::unique_ptr<jsmntok_t[]> tokens_dynamic;
    jsmntok_t* tokens = tokens_static;
    int parsed;

    while (true) {
        jsmn_init(&parser);
        parsed = jsmn_parse(&parser, buf, size, tokens, tokens_size);
        if (parsed >= 0) {
            break;
        } else if (parsed == JSMN_ERROR_NOMEM) {
            tokens_size *= 2;
            if (tokens_size >= 128) {
                ESP_LOGE(TAG, "failed to parse msg %.*s: too big", size, buf);
                return NULL;
            }
            tokens_dynamic.reset(new jsmntok_t[tokens_size]);
            tokens = tokens_dynamic.get();
        } else {
            ESP_LOGE(TAG, "failed to parse msg %.*s: %d", size, buf, parsed);
            return NULL;
        }
    }
    return parse_object(buf, &tokens[0]);
}

Value::Value(Value::type_t type)
    : m_type(type) {
}

Value::~Value() {
}

std::string Value::str() const {
    std::ostringstream ss;
    serialize(ss);
    return ss.str();
}

Object::Object()
    : Value(Value::OBJECT) {
}

Object::~Object() {
    for (auto itr = m_members.begin(); itr != m_members.end(); ++itr) {
        delete itr->second;
    }
}

void Object::serialize(std::ostream& ss) const {
    ss.put('{');
    for (auto itr = m_members.cbegin(); itr != m_members.cend();) {
        write_string_escaped(itr->first.c_str(), ss);
        ss.put(':');
        itr->second->serialize(ss);
        if (++itr != m_members.cend()) {
            ss.put(',');
        }
    }
    ss.put('}');
}

void Object::swapData(Object& other) {
    m_members.swap(other.m_members);
}

bool Object::equals(const Value& other) const {
    if (!Value::equals(other))
        return false;

    const auto& obj = static_cast<const Object&>(other);
    if (m_members.size() != obj.m_members.size())
        return false;

    for (const auto& pair : m_members) {
        const auto itr = obj.m_members.find(pair.first);
        if (itr == obj.m_members.end() || !itr->second->equals(*pair.second))
            return false;
    }
    return true;
}

Value* Object::copy() const {
    auto* res = new Object();
    for (const auto& pair : m_members) {
        res->m_members[pair.first] = pair.second->copy();
    }
    return res;
}

bool Object::contains(const std::string& key) const {
    return m_members.find(key) != m_members.end();
}

Value* Object::get(const std::string& key) const {
    const auto itr = m_members.find(key);
    if (itr == m_members.cend())
        return NULL;
    return itr->second;
}

Object* Object::getObject(const std::string& key) const {
    auto* val = get(key);
    if (val && val->getType() == OBJECT) {
        return (Object*)val;
    }
    return NULL;
}

Array* Object::getArray(const std::string& key) const {
    auto* val = get(key);
    if (val && val->getType() == ARRAY) {
        return (Array*)val;
    }
    return NULL;
}

std::string Object::getString(const std::string& key, std::string def) const {
    auto* val = get(key);
    if (val && val->getType() == STRING) {
        return ((String*)val)->get();
    } else {
        return def;
    }
}

int64_t Object::getInt(const std::string& key, int64_t def) const {
    auto* val = get(key);
    if (val && val->getType() == NUMBER) {
        return ((Number*)val)->get();
    } else {
        return def;
    }
}

double Object::getDouble(const std::string& key, double def) const {
    auto* val = get(key);
    if (val && val->getType() == NUMBER) {
        return ((Number*)val)->get();
    } else {
        return def;
    }
}

bool Object::getBool(const std::string& key, bool def) const {
    auto* val = get(key);
    if (val && val->getType() == BOOL) {
        return ((Bool*)val)->get();
    } else {
        return def;
    }
}

void Object::set(const std::string& key, Value* value) {
    auto itr = m_members.find(key);
    if (itr != m_members.end()) {
        delete itr->second;
        itr->second = value;
    } else {
        m_members[key] = value;
    }
}

void Object::set(const std::string& key, const std::string& string) {
    set(key, new String(string));
}

void Object::set(const std::string& key, double number) {
    set(key, new Number(number));
}

void Object::remove(const std::string& key) {
    auto itr = m_members.find(key);
    if (itr != m_members.end()) {
        delete itr->second;
        m_members.erase(itr);
    }
}

Array::Array()
    : Value(Value::ARRAY) {
}

Array::~Array() {
    for (auto val : m_items) {
        delete val;
    }
}

void Array::serialize(std::ostream& ss) const {
    ss.put('[');
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_items[i]->serialize(ss);
        if (i + 1 != m_items.size()) {
            ss.put(',');
        }
    }
    ss.put(']');
}

bool Array::equals(const Value& other) const {
    if (!Value::equals(other))
        return false;

    const auto& array = static_cast<const Array&>(other);
    if (m_items.size() != array.m_items.size())
        return false;

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i]->equals(*array.m_items[i]))
            return false;
    }
    return true;
}

Value* Array::copy() const {
    auto* res = new Array();
    res->m_items.reserve(m_items.size());
    for (const auto& it : m_items) {
        res->m_items.push_back(it->copy());
    }
    return res;
}

Value* Array::get(size_t idx) const {
    if (idx < m_items.size())
        return m_items[idx];
    return NULL;
}

Object* Array::getObject(size_t idx) const {
    auto* val = get(idx);
    if (val && val->getType() == OBJECT) {
        return (Object*)val;
    }
    return NULL;
}

Array* Array::getArray(size_t idx) const {
    auto* val = get(idx);
    if (val && val->getType() == ARRAY) {
        return (Array*)val;
    }
    return NULL;
}

std::string Array::getString(size_t idx, std::string def) const {
    auto* val = get(idx);
    if (val && val->getType() == STRING) {
        return ((String*)val)->get();
    } else {
        return def;
    }
}

int64_t Array::getInt(size_t idx, int64_t def) const {
    auto* val = get(idx);
    if (val && val->getType() == NUMBER) {
        return ((Number*)val)->get();
    } else {
        return def;
    }
}

double Array::getDouble(size_t idx, double def) const {
    auto* val = get(idx);
    if (val && val->getType() == NUMBER) {
        return ((Number*)val)->get();
    } else {
        return def;
    }
}

bool Array::getBool(size_t idx, bool def) const {
    auto* val = get(idx);
    if (val && val->getType() == BOOL) {
        return ((Bool*)val)->get();
    } else {
        return def;
    }
}

void Array::set(size_t idx, Value* value) {
    if (idx < m_items.size()) {
        delete m_items[idx];
        m_items[idx] = value;
    }
}

void Array::insert(size_t idx, Value* value) {
    m_items.insert(m_items.begin() + idx, value);
}

void Array::remove(size_t idx) {
    if (idx < m_items.size()) {
        delete m_items[idx];
        m_items.erase(m_items.begin() + idx);
    }
}

String::String(const char* value)
    : Value(STRING)
    , m_value(value) {
}

String::String(const std::string& value)
    : Value(STRING)
    , m_value(value) {
}

String::~String() {
}

void String::serialize(std::ostream& ss) const {
    write_string_escaped(m_value.c_str(), ss);
}

bool String::equals(const Value& other) const {
    if (!Value::equals(other))
        return false;

    const auto& str = static_cast<const String&>(other);
    return m_value == str.m_value;
}

Value* String::copy() const {
    return new String(m_value);
}

Number::Number(double value)
    : Value(NUMBER)
    , m_value(value) {
}

Number::~Number() {
}

void Number::serialize(std::ostream& ss) const {
    float intpart;
    float fracpart = fabsf(modff(m_value, &intpart));
    if (fracpart < 0.0001f) {
        fctprintf(fct_ostream, &ss, "%lld", (long long)m_value);
    } else {
        // std::stringstream needs 1.5KB of stack to format a double
        fctprintf(fct_ostream, &ss, "%.4f", m_value);
    }
}

bool Number::equals(const Value& other) const {
    if (!Value::equals(other))
        return false;

    const auto& num = static_cast<const Number&>(other);
    return m_value == num.m_value;
}

Value* Number::copy() const {
    return new Number(m_value);
}

Bool::Bool(bool value)
    : Value(BOOL)
    , m_value(value) {
}

Bool::~Bool() {
}

void Bool::serialize(std::ostream& ss) const {
    if (m_value) {
        ss.write("true", 4);
    } else {
        ss.write("false", 5);
    }
}

bool Bool::equals(const Value& other) const {
    if (!Value::equals(other))
        return false;

    const auto& boolean = static_cast<const Bool&>(other);
    return m_value == boolean.m_value;
}

Value* Bool::copy() const {
    return new Bool(m_value);
}

void Nil::serialize(std::ostream& ss) const {
    ss.write("null", 4);
}

Value* Nil::copy() const {
    return new Nil();
}

};
