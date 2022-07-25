#include <iostream>
#include <cstring>

class String;

bool operator==(const String&, const String&);

class String {
private:
    size_t sz = 0;
    size_t reserved_sz = 0;
    char* str = nullptr;

private:
    void swap(String& s) {
        std::swap(sz, s.sz);
        std::swap(reserved_sz, s.reserved_sz);
        std::swap(str, s.str);
    }

public:
    String() : sz(0), reserved_sz(1), str(new char[reserved_sz]) {}

    String(char c) : sz(1), reserved_sz(sz), str(new char[reserved_sz]) {
        memset(str, c, sz);
    }

    String(const char* s) : sz(strlen(s)), reserved_sz(sz), str(new char[reserved_sz]) {
        memcpy(str, s, sz);
    }

    String(size_t n, char c) : sz(n), reserved_sz(sz), str(new char[reserved_sz]) {
        memset(str, c, sz);
    }

    String(const String& s) : sz(s.sz), reserved_sz(sz), str(new char[reserved_sz]) {
        memcpy(str, s.str, sz);
    }

    const char& operator[](size_t i) const {
        return *(str + i);
    }

    char& operator[](size_t i) {
        return *(str + i);
    }

    String& operator=(String s) {
        swap(s);
        return *this;
    }

    size_t length() const {
        return sz;
    }

    bool empty() const {
        return sz == 0;
    }

    String& push_back(char c) {
        if (sz == reserved_sz) {
            String str_(*this);
            delete[] str;
            reserved_sz = 2 * reserved_sz + 1;
            str = new char[reserved_sz];
            memcpy(str, str_.str, sz);
        }
        str[sz++] = c;
        return *this;
    }

    String& pop_back() {
        sz--;
        return *this;
    }

    char& front() {
        return str[0];
    }
    
    const char& front() const {
        return str[0];
    }

    char& back() {
        return str[sz - 1];
    }

    const char& back() const {
        return str[sz - 1];
    }

    String& operator+=(const String& s) {
        size_t s_sz = s.length();
        if (sz + s_sz > reserved_sz) {
            String str_(*this);
            delete[] str;
            reserved_sz = 2 * (sz + s_sz) + 1;
            str = new char[reserved_sz];
            memcpy(str, str_.str, sz);
        }
        for (size_t i = 0; i < s_sz; i++) {
            str[sz++] = s[i];
        }
        return *this;
    }

    String& operator+=(char c) {
        push_back(c);
        return *this;
    }

    String substr(size_t start, size_t count) const {
        String copy;
        for (size_t i = 0; i < count; i++) {
            copy += str[start + i];
        }
        return copy;
    }

    size_t find(const String& s) const {
        for (size_t i = 0; i + s.length() <= length(); i++) {
            if (substr(i, s.length()) == s) {
                return i;
            }
        }
        return length();
    }

    size_t rfind(const String& s) const {
        if (length() < s.length()) {
            return length();
        }
        for (size_t i = length() - s.length(); ; i--) {
            if (substr(i, s.length()) == s) {
                return i;
            }
            if (i == 0) break;
        }
        return length();
    }

    void clear() {
        sz = 0;
    }

    ~String() {
        delete[] str;
    }
};

String operator+(const String& lhs, const String& rhs) {
    String copy = lhs;
    copy += rhs;
    return copy;
}

bool operator==(const String& lhs, const String& rhs) {
    if (lhs.length() != rhs.length()) return false;
    for (size_t i = 0; i < lhs.length(); i++) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& out, const String& s) {
    for (size_t i = 0; i < s.length(); i++) {
        out << s[i];
    }
    return out;
}

std::istream& operator>>(std::istream& in, String& str) {
    char c;
    str.clear();
    while (1) {
        c = in.get();
        if (!isspace(c) || c == EOF) break;
    }
    while (1) {
        if (isspace(c) || c == EOF) break;
        str += c;
        c = in.get();
    }
    return in;
}