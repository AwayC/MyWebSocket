#pragma once

#include <cassert>
#include <stddef.h>
#include <string>
#include <vector>
#include <map>
#include <initializer_list>
enum class lept_type { null, lfalse, ltrue, number, string, array, object };

class lept_value;

using object_t = std::map<std::string, lept_value>;
using array_t = std::vector<lept_value>;

class lept_value {
private:
	union u{
		double n;
		std::string s;
		array_t arr;
		object_t obj;
		u() {};
		~u() {};
	};
	lept_type type;
	u v;

	void free();
	void stringify_value(std::string &stk) const;
	void stringify_string(std::string& stk) const;

public :
	lept_value() noexcept ;
	lept_value(const lept_value& val);
	lept_value(const std::string& s);
	lept_value(std::string&& s);
	lept_value(double d);
	lept_value(int i) : lept_value((double)i) {}
	lept_value(std::vector<lept_value>&& arr);
	lept_value(std::map<std::string, lept_value>&& obj);
	lept_value(std::nullptr_t) noexcept;
	lept_value(const char* str) : lept_value(std::string(str)) {}
	lept_value(bool b)
	{
		this->set_boolean(b);
	}

	lept_value(std::initializer_list<lept_value> initList);
	lept_value& operator=(lept_value val)
	{
		std::swap(*this, val);
		return *this;
	}

	~lept_value() noexcept;

	int parse(std::string json);

	static std::string typeStr(lept_type t);

	void set_null();

	lept_type get_type() {
		return type;
	};

	bool get_boolean();
	void set_boolean(int b);
	void set_boolean(bool b)
	{
		set_boolean(b ? 1 : 0);
	}

	double get_number();
	void set_number(double num);

	const std::string& get_string();
	void set_string(std::string);

	size_t get_array_size();
	lept_value& get_array_element(size_t index);
	const lept_value& get_element(size_t index);
	void set_array(std::vector<lept_value>&& val);
	void set_array(const array_t& arr);

	bool contains_key(std::string key);
	lept_value get_object_value(std::string key);
	size_t get_object_size();
	void set_object(object_t&& obj);
	void set_object(const std::map<std::string, lept_value>& mp);

	lept_value& operator[](const std::string& key)
	{
		assert(type == lept_type::object);
		return v.obj[key];
	}

	std::string stringify() const;
};

enum  {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_INVALID_UNICODE_HEX,
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
	LEPT_PARSE_MISS_KEY,
	LEPT_PARSE_MISS_COLON,
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};