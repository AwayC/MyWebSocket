#include "leptjson.h"
#include <assert.h>
#include <math.h>
#include <string>
#include <utility>
#include <vector>
#include <errno.h>
#include <iostream>
#include "double-conversion.h"
using namespace double_conversion;

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

class lept_context{
public :
	std::string json;
	size_t ptr;

	size_t  str_top;
	std::string stk_str;

	void parse_whitespace();
	int parse_value(lept_value* v);
	int parse_literal(lept_value* v, std::string literal , lept_type type);
	int parse_number(lept_value* v);
	int parse_string(lept_value* v);
	size_t parse_hex4(size_t p , int* u);
	int parse_array(lept_value* v);
	int parse_object(lept_value* v);
	void encode_utf8(int u);
	void push_char(char c);
	std::string pop_string(size_t size);

	lept_context() { ptr = str_top = 0; stk_str = ""; };
};


void lept_context::push_char(char c) {
	str_top++;
	stk_str.push_back(c);
}

std::string lept_context::pop_string(size_t size) {
	assert(str_top >= size);
	return stk_str;
}

void lept_context::parse_whitespace() {
	size_t tmp = this->ptr;
	while (json[tmp] == ' ' || json[tmp] == '\t' || json[tmp] == '\n' || json[tmp] == '\r')
		tmp++;
	ptr = tmp;
}


int lept_context::parse_literal(lept_value* v, std::string literal, lept_type type) {
	assert(ptr < json.size() && json[ptr] == literal[0]);
	int i;
	for (i = 0; i < literal.size(); i++) {
		if (json[i + ptr] != literal[i])
			return LEPT_PARSE_INVALID_VALUE;
	}
	ptr += i;
	switch (type) {
		case lept_type::null: v->set_null(); break;
		case lept_type::lfalse: v->set_boolean(0); break;
		case lept_type::ltrue: v->set_boolean(1);  break;
		default: break;
	}
	return LEPT_PARSE_OK;
}


int lept_context::parse_number(lept_value* v) {
	size_t tmp = ptr;
	bool is_integer = true;

	if (json[tmp] == '-') tmp++;
	if (json[tmp] == '0') tmp++;
	else {
		if (!ISDIGIT1TO9(json[tmp])) return LEPT_PARSE_INVALID_VALUE;
		for (tmp++; ISDIGIT(json[tmp]); tmp++);
	}
	if (json[tmp] == '.') {
		is_integer = false;
		tmp++;
		if (!ISDIGIT(json[tmp])) return LEPT_PARSE_INVALID_VALUE;
		for (tmp++; ISDIGIT(json[tmp]); tmp++);
	}
	if (json[tmp] == 'e' || json[tmp] == 'E') {
		is_integer = false;
		tmp++;
		if (json[tmp] == '+' || json[tmp] == '-') tmp++;
		if (!ISDIGIT(json[tmp])) return LEPT_PARSE_INVALID_VALUE;
		for (tmp++; ISDIGIT(json[tmp]); tmp++);
	}

	if (is_integer)
	{
		try {
			long long int_val = std::stoll(json.substr(ptr, tmp - ptr));
			v->set_integer(int_val);
		}
		catch (const std::out_of_range& e) {
			return LEPT_PARSE_NUMBER_TOO_BIG;
		}
	} else
	{
		const int flags = StringToDoubleConverter::ALLOW_TRAILING_JUNK |
							   StringToDoubleConverter::ALLOW_LEADING_SPACES;
		StringToDoubleConverter converter(flags, 0.0, 0.0, "inf", "nan");
		int processed_characters_count;
		double num = converter.StringToDouble(json.substr(ptr, tmp - ptr).c_str() , tmp - ptr, &processed_characters_count);

		/*if (errno == ERANGE && (num == HUGE_VAL || num == -HUGE_VAL))
			return LEPT_PARSE_NUMBER_TOO_BIG;*/
		if (std::isinf(num) || std::isnan(num))
			return LEPT_PARSE_NUMBER_TOO_BIG;
		v->set_number(num);
		//double num = stod(json.substr(ptr, tmp - ptr));
	}

	ptr = tmp;
	return LEPT_PARSE_OK;
}

size_t lept_context::parse_hex4(size_t p, int* u) {
	*u = 0;
	for (int i = 0; i < 4; i++) {
		char ch = json[++ p];
		*u <<= 4;
		if (ch >= '0' && ch <= '9') *u |= ch - '0';
		else if (ch >= 'A' && ch <= 'Z') *u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'z') *u |= ch - ('a' - 10);
		else return 0;
	}
	return p;
}

void lept_context::encode_utf8(int u) {
	if (u <= 0x7F)
		push_char((char)u);
	else if (u <= 0x7FF) {
		push_char((char)(0xC0 | ((u >> 6) & 0xFF)));
		push_char((char)(0x80 | (u) & 0x3F));
	}
	else if (u <= 0xFFFF) {
		push_char((char)(0xE0 | (u >> 12) & 0xFF));
		push_char((char)(0x80 | (u >> 6) & 0x3F));
		push_char((char)(0x80 | u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		push_char((char)(0xF0 | (u >> 18) & 0xFF));
		push_char((char)(0x80 | (u >> 12) & 0x3F));
		push_char((char)(0x80 | (u >> 6) & 0x3F));
		push_char((char)(0x80 | u & 0x3F));
	}
}

int lept_context::parse_string(lept_value* v) {
	assert(ptr < json.size() && json[ptr] == '\"');
	size_t tmp = ptr, len = 0, head = str_top;
	std::string str;
	int u, u2;
	for (;;) {
		char ch = json[++tmp];
		switch (ch) {
			case '\"':
				len = str_top - head;
				v->set_string(pop_string(len));
				ptr = ++tmp;
				str_top = head;
				stk_str.clear();
				return LEPT_PARSE_OK;
			case '\\':
				switch (json[++tmp]) {
					case '\"': push_char('\"');  break;
					case '\\': push_char('\\'); break;
					case '/': push_char('/');  break;
					case 'b': push_char('\b'); break;
					case 't': push_char('\t'); break;
					case 'n': push_char('\n'); break;
					case 'r':push_char('\r'); break;
					case 'f': push_char('\f'); break;
					case 'u':
						if (!(tmp = parse_hex4(tmp, &u)))
							return LEPT_PARSE_INVALID_UNICODE_HEX;
						if (u >= 0xD800 && u <= 0xDBFF) {
							if (json[++tmp] != '\\')
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							if (json[++tmp] != 'u')
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							if (!(tmp = parse_hex4(tmp, &u2)))
								return LEPT_PARSE_INVALID_UNICODE_HEX;
							if (u2 < 0xDC00 || u2 > 0xDFFF)
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
						}
						encode_utf8(u);
						break;
					default:
						return LEPT_PARSE_INVALID_STRING_CHAR;
				}
				break;
			case '\0':
				str_top = 0;
				stk_str.clear();
				return LEPT_PARSE_MISS_QUOTATION_MARK;
			default:
				if ((unsigned char)ch < 0x20) {
					str_top = 0;
					stk_str.clear();
					return LEPT_PARSE_INVALID_STRING_CHAR;
				}
				push_char(ch);
				break;
		}
	}
}

int lept_context::parse_array(lept_value* v) {
	assert(json[ptr ++] == '[');
	std::vector<lept_value> arr;
	parse_whitespace();
	if (json[ptr] == ']') {
		ptr++;
		v->set_array(std::move(arr));
		return LEPT_PARSE_OK;
	}
	int ret;
	size_t len = 0;
	for (;;) {
		lept_value e;
		if ((ret = parse_value(&e)) != LEPT_PARSE_OK)
			break;
		arr.push_back(e);
		len++;
		parse_whitespace();
		if (json[ptr] == ',') {
			ptr++;
			parse_whitespace();
		}
		else if (json[ptr] == ']') {
			ptr++;
			v->set_array(std::move(arr));
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	return ret;
}

int lept_context::parse_object(lept_value* v) {
	assert(json[ptr ++] == '{');
	std::map<std::string, lept_value> mp;
	parse_whitespace();
	if (json[ptr] == '}') {
		v->set_object(std::move(mp));
		ptr++;
		return LEPT_PARSE_OK;
	}
	int ret;
	for (;;) {
		if (json[ptr] != '\"')
			return LEPT_PARSE_MISS_KEY;
		lept_value str;
		if ((ret = parse_string(&str)) != LEPT_PARSE_OK)
			return ret;
		parse_whitespace();
		if (json[ptr] != ':')
			return LEPT_PARSE_MISS_COLON;
		else {
			ptr++;
			parse_whitespace();
		}
		lept_value e;
		if ((ret = parse_value(&e)) != LEPT_PARSE_OK)
			return ret;
		mp.insert(std::pair<std::string, lept_value>(str.get_string(), e));
		parse_whitespace();
		if (json[ptr] == '}') {
			v->set_object(std::move(mp));
			ptr++;
			return LEPT_PARSE_OK;
		}
		else if (json[ptr] == ',') {
			ptr++;
			parse_whitespace();
		}
		else {
			return LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
		}
	}
}

int lept_context::parse_value(lept_value* v) {
	switch (this->json[ptr]) {
	case 't': return this->parse_literal(v, "true", lept_type::ltrue);
	case 'f': return this->parse_literal(v, "false", lept_type::lfalse);
	case 'n': return this->parse_literal(v, "null", lept_type::null);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	case '\"': return this->parse_string(v);
	case '[': return this->parse_array(v);
	case '{': return this->parse_object(v);
	default: return parse_number(v);
	}
}

/**********************************  lept_value  **************************************/

std::string lept_value::typeStr(lept_type t)
{
#define CASE_(x, s) case lept_type::x: return #s;
		switch (t) {
			CASE_(null, null)
			CASE_(lfalse, false)
			CASE_(ltrue, true)
			CASE_(number, number)
			CASE_(string, string)
			CASE_(array, array)
			CASE_(object, object)
			default: return "unknown";
		}
#undef CASE_(x, s)
}

int lept_value::parse(std::string json) {
	lept_context c;
	int ret;
	c.json = json + '\0';
	c.str_top = 0;
	this->free();
	c.parse_whitespace();
	if (ret = c.parse_value(this)) {
		c.parse_whitespace();
		if (c.ptr != c.json.size() - 1) {
			this->type = lept_type::null;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.str_top == 0);
	c.stk_str.clear();
	return ret;
}

lept_value::lept_value() noexcept{
	this->type = lept_type::null;
}

lept_value::lept_value(const lept_value& val) {
	switch (val.type) {
		case lept_type::number: v.n = val.v.n; break;
		case lept_type::integer: v.i = val.v.i; break;
		case lept_type::string: new(&v.s) std::string(val.v.s); break;
		case lept_type::array: new(&v.arr) array_t(val.v.arr); break;
		case lept_type::object: new(&v.obj) object_t(val.v.obj); break;
		default: break;
	}
	type = val.type;
}

lept_value& lept_value::operator=(lept_value val) {
	this->free();

	switch (val.type) {
		case lept_type::number: v.n = val.v.n; break;
		case lept_type::integer: v.i = val.v.i; break;
		case lept_type::string: new(&v.s) std::string(std::move(val.v.s)); break;
		case lept_type::array: new(&v.arr) array_t(std::move(val.v.arr)); break;
		case lept_type::object: new(&v.obj) object_t(std::move(val.v.obj)); break;
		default: break;
	}

	type = val.type;
	return *this;
}

lept_value::~lept_value() noexcept{
	this->free();
}

void lept_value::free() {
	switch (this->type) {
		case lept_type::string:
			v.s.~basic_string(); break;
		case lept_type::array:
			v.arr.~vector(); break;
		case lept_type::object :
			v.obj.~map();
		default:
			break;
	}
	type = lept_type::null;
}

void lept_value::set_null() {
	this->free();
}

void lept_value::set_boolean(int b) {
	this->free();
	type = b == 0 ? lept_type::lfalse : lept_type::ltrue;
}

bool lept_value::get_boolean() const{
	assert(type == lept_type::lfalse || type == lept_type::ltrue);
	if (type == lept_type::ltrue) return true;
	else return false;
}

void lept_value::set_number(double num) {
	this->free();
	this->type = lept_type::number;
	this->v.n = num;
}

double lept_value::get_number() const{
	assert(this->type == lept_type::number);
	return v.n;
}

void lept_value::set_integer(int64_t i)
{
	this->free();
	this->type = lept_type::integer;
	this->v.i = i;
}

int64_t lept_value::get_integer() const
{
	assert(this->type == lept_type::integer);
	return v.i;
}

const std::string& lept_value::get_string() const {
	assert(type == lept_type::string);
	return v.s;
}

void lept_value::set_string(std::string str) {
	this->free();
	new(&v.s) std::string(std::move(str));
	type = lept_type::string;
}

void lept_value::set_array(std::vector<lept_value>&& val) {
	this->free();
	type = lept_type::array;
	new(&v.arr) std::vector<lept_value>(std::move(val));
}

void lept_value::set_array(const array_t& arr) {
	this->free();
	type = lept_type::array;
	new(&v.arr) std::vector<lept_value>(arr);
}

size_t lept_value::get_array_size() const {
	assert(type == lept_type::array);
	return v.arr.size();
}

lept_value& lept_value::get_array_element(size_t index) {
	assert(type == lept_type::array && v.arr.size() > index);
	return v.arr[index];
}

const lept_value& lept_value::get_element(size_t index) const {
	assert(type == lept_type::array && v.arr.size() > index);
	return v.arr[index];
}

bool lept_value::contains_key(std::string key) {
	return (v.obj.count(key) != 0);
}

lept_value lept_value::get_object_value(std::string key) {
	assert(v.obj.count(key) > 0);
	return v.obj[key];
}

size_t lept_value::get_object_size() const {
	assert(type == lept_type::object);
	return v.obj.size();
}

void lept_value::set_object(std::map<std::string, lept_value>&& mp) {
	this->free();
	type = lept_type::object;
	new(&v.obj) std::map<std::string, lept_value>(std::move(mp));
}

void lept_value::set_object(const std::map<std::string, lept_value>& mp) {
	this->free();
	type = lept_type::object;
	// mp 是一个左值引用，这里只能调用拷贝构造函数
	new(&v.obj) std::map<std::string, lept_value>(mp);
}

void lept_value::stringify_string(std::string& stk) const {
	stk += '\"';
	for (int i = 0; i < v.s.size(); i++) {
		char ch = v.s[i];
		switch (ch) {
			case '\"': stk += "\\\""; break;
			case '/': stk += "\\/"; break;
			case '\\': stk += "\\\\"; break;
			case '\b': stk += "\\b"; break;
			case '\t': stk += "\\t"; break;
			case '\r': stk += "\\r"; break;
			case '\f': stk += "\\f"; break;
			case '\n': stk += "\\n"; break;
			default:
				if (ch < 0x20) {
					char buff[7];
					sprintf(buff, "\\u%04X", ch);
					stk += buff;
				}
				else
					stk += v.s[i];
				break;
		}
	}
	stk += '\"';
}

void lept_value::stringify_value(std::string& stk) const {
	int flag;
	switch (type) {
		case lept_type::null: stk.append("null"); break;
		case lept_type::ltrue: stk.append( "true"); break;
		case lept_type::lfalse: stk.append( "false"); break;
		case lept_type::number:
		{
			char s[32];
			sprintf(s, "%.17g", v.n);
			stk += s;
		}
			break;
		case lept_type::integer:
		{
			char s[32];
			sprintf(s, "%ld", v.i);
			stk += s;
		}
			break;
#if 1
		case lept_type::string: stringify_string(stk); break;
#endif
#if 1
		case lept_type::array:
			stk.push_back('[');
			flag = 0;
			for (auto val : v.arr) {
				if (flag) stk.push_back(',');
				else flag |= 1;
				val.stringify_value(stk);
			}
			stk.push_back(']');
			break;
#endif
#if 1
		case lept_type::object:
			stk.push_back('{');
			flag = 0;
			for (auto &item : v.obj) {
				if (flag) stk.push_back(',');
				else flag |= 1;
				stk.push_back('\"');
				stk.append(item.first);
				stk.push_back('\"');
				stk.push_back(':');
				item.second.stringify_value(stk);
			}
			stk.push_back('}');
			break;
#endif
		default:
			break;
	}
}

std::string lept_value::stringify() const{
	std::string stk;
	stringify_value(stk);
	return stk;
}

lept_value::lept_value(const std::string& s)
{
	this->type = lept_type::string;
	new(&v.s) std::string(s);
}

lept_value::lept_value(std::string&& s)
{
	this->type = lept_type::string;
	new(&v.s) std::string(std::move(s));
}

lept_value::lept_value(double d)
{
	this->type = lept_type::number;
	this->v.n = d;
}

lept_value::lept_value(int i)
{
	this->type = lept_type::integer;
	this->v.i = i;
}

lept_value::lept_value(int64_t i)
{
	this->type = lept_type::integer;
	this->v.i = i;
}

lept_value::lept_value(std::vector<lept_value>&& arr)
{
	this->type = lept_type::array;
	new(&v.arr) std::vector<lept_value>(arr);
}
lept_value::lept_value(std::map<std::string, lept_value>&& obj)
{
	this->type = lept_type::object;
	new(&v.obj) std::map<std::string, lept_value>(obj);
}


lept_value::lept_value(std::nullptr_t) noexcept
{
	this->type = lept_type::null;
}


lept_value::lept_value(std::initializer_list<lept_value> initList)
{
	bool is_an_object = std::all_of(initList.begin(), initList.end(),
		[](const lept_value& ele)
		{
			return ele.type == lept_type::array && ele.v.arr.size() == 2
				&& ele.v.arr[0].type == lept_type::string;
		});
	if (is_an_object)
	{
		object_t obj;
		for (auto &it : initList)
		{
			obj.emplace(it.v.arr[0].v.s, it.v.arr[1]);
		}

		this->set_object(std::move(obj));
	} else
	{
		this->set_array(array_t(initList));
	}
}
