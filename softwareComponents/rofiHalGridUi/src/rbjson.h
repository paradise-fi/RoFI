#pragma once

#include <map>
#include <sstream>
#include <vector>

/**
 * \brief JSON-related objects
 */
namespace rbjson {

class Object;

/**
 * \brief Parse a JSON string to an object.
 */
Object* parse(char* buf, size_t size);

/**
 * \brief Base JSON value class, not instanceable.
 */
class Value {
public:
    enum type_t : uint8_t {
        OBJECT,
        ARRAY,
        STRING,
        NUMBER,
        BOOL,
        NIL
    };

    Value(type_t type = NIL);
    virtual ~Value();

    virtual void serialize(std::ostream& ss) const = 0; //!< Serialize the value to a string
    std::string str() const; //!< Helper that calls serialize() and returns a string

    //!< Get the object type
    type_t getType() const {
        return m_type;
    }

    //!< Return true if the object is of type NIL
    bool isNil() const {
        return m_type == NIL;
    }

    virtual bool equals(const Value& other) const {
        return m_type == other.m_type;
    }

    virtual Value* copy() const = 0;

protected:
    type_t m_type;
};

class Array;

/**
 * \brief A JSON Object
 */
class Object : public Value {
public:
    static Object* parse(char* buf, size_t size);

    Object();
    ~Object();

    void serialize(std::ostream& ss) const;
    bool equals(const Value& other) const;
    Value* copy() const;

    void swapData(Object& other);

    bool contains(const std::string& key) const;
    const std::map<std::string, Value*>& members() const { return m_members; }

    Value* get(const std::string& key) const;
    Object* getObject(const std::string& key) const;
    Array* getArray(const std::string& key) const;
    std::string getString(const std::string& key, std::string def = "") const;
    int64_t getInt(const std::string& key, int64_t def = 0) const;
    double getDouble(const std::string& key, double def = 0.0) const;
    bool getBool(const std::string& key, bool def = false) const;

    void set(const std::string& key, Value* value);
    void set(const std::string& key, const std::string& str);
    void set(const std::string& key, double number);

    void remove(const std::string& key);

private:
    std::map<std::string, Value*> m_members;
};

/**
 * \brief A JSON Array
 */
class Array : public Value {
public:
    Array();
    ~Array();

    void serialize(std::ostream& ss) const;
    bool equals(const Value& other) const;
    Value* copy() const;

    size_t size() const { return m_items.size(); };

    Value* get(size_t idx) const;
    Object* getObject(size_t idx) const;
    Array* getArray(size_t idx) const;
    std::string getString(size_t idx, std::string def = "") const;
    int64_t getInt(size_t idx, int64_t def = 0) const;
    double getDouble(size_t idx, double def = 0.0) const;
    bool getBool(size_t idx, bool def = false) const;

    void set(size_t idx, Value* value);
    void insert(size_t idx, Value* value);
    void push_back(Value* value) {
        insert(m_items.size(), value);
    }
    void remove(size_t idx);

private:
    std::vector<Value*> m_items;
};

/**
 * \brief A JSON String
 */
class String : public Value {
public:
    explicit String(const char* value = "");
    explicit String(const std::string& value);
    ~String();

    void serialize(std::ostream& ss) const;
    bool equals(const Value& other) const;
    Value* copy() const;

    const std::string& get() const { return m_value; };

private:
    std::string m_value;
};

/**
 * \brief A JSON Number. It's a double internally.
 */
class Number : public Value {
public:
    explicit Number(double value = 0.0);
    ~Number();

    void serialize(std::ostream& ss) const;
    bool equals(const Value& other) const;
    Value* copy() const;

    double get() const { return m_value; };

private:
    float m_value;
};

/**
 * \brief JSON Boolean value
 */
class Bool : public Value {
public:
    explicit Bool(bool value = false);
    ~Bool();

    void serialize(std::ostream& ss) const;
    bool equals(const Value& other) const;
    Value* copy() const;

    bool get() const { return m_value; };

private:
    bool m_value;
};

/**
 * \brief JSON Nil(null) value.
 */
class Nil : public Value {
public:
    void serialize(std::ostream& ss) const;
    Value* copy() const;
};

};
