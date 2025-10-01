#pragma once 

#include <stddef.h>
#include <string> 
#include <vector> 
#include <map> 
enum class lept_type { null, lfalse, ltrue, number, string, array, object };

class lept_value;

class lept_value {
private: 
	union u{
		double n;
		std::string s;
		std::vector<lept_value> arr;
		std::map<std::string, lept_value> obj;
		u() {}; 
		~u() {}; 
	};
	lept_type type;
	u v; 

	void free(); 
	void stringify_value(std::string &stk); 
	void stringify_string(std::string& stk); 
public : 
	lept_value() noexcept ;
	lept_value(const lept_value& val); 
	~lept_value() noexcept;
	
	int parse(std::string json);
	

	void set_null(); 

	lept_type get_type() {
		return type;
	};

	bool get_boolean(); 
	void set_boolean(int b);

	double get_number(); 
	void set_number(double num);

	const std::string& get_string();
	void set_string(std::string);

	size_t get_array_size();
	lept_value& get_array_element(size_t index);
	const lept_value& get_element(size_t index); 
	void set_array(std::vector<lept_value>&& val);

	bool contains_key(std::string key);
	lept_value get_object_value(std::string key); 
	size_t get_object_size(); 
	void set_object(const std::map<std::string, lept_value> mp); 

	std::string stringify(); 
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