#include <iostream>
#include <vector>
#include <complex>

namespace BigNumber {
const double PI = acos(-1.0);

enum class Sign {
    plus = 1,
    minus = -1
};

Sign operator*(Sign x, Sign y) {
    if (x == y) return Sign::plus;
    else return Sign::minus;
}

class BigInteger {
private:
    std::vector<long long> a;
    Sign sign = Sign::plus;
    static const long long base = 1000000;
    static const size_t base_len = 6;

private:
    static std::string number_to_string(long long x) {
        std::string result = std::to_string(x);
        return std::string(base_len - result.size(), '0') + result;
    }
    
    static void reverse(std::string& s) {
        if (s.size() == 0) return;
        for (size_t curl = 0, curr = s.size() - 1; curl < curr; curl++, curr--) {
            std::swap(s[curl], s[curr]);
        }
    }

    static void delete_trailing_zeroes(std::string& s) {
        reverse(s);
        while (s.size() > 1 && s.back() == '0') s.pop_back();
        reverse(s);
    }

    static void delete_trailing_zeroes(std::vector<long long>& x) {
        while (x.size() > 1 && x.back() == 0) x.pop_back();
    }

    static Sign sgn(long long x) {
        return static_cast<Sign>((0 <= x) - (x < 0));
    }

    static void FFT(std::vector<std::complex<double>>& a, bool invert) {
        long long n = a.size();
        for (int i = 1, j = 0; i < n - 1; i++) {
            for (int s = n; j ^= s >>= 1, ~j & s;);
            if (i < j) swap(a[i], a[j]);
        }
        for (int len = 2; len <= n; len <<= 1) {
            double angle = 2 * PI / len * (invert ? -1 : 1);
            std::complex<double> wlen(cos(angle), sin(angle)), w, u, v;
            for (int i = 0; i < n; i += len) {
                w = 1;
                for (int j = 0; j < len / 2; j++) {
                    u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
        if (invert) {
            for (int i = 0; i < n; ++i) {
                a[i] /= n;
            }
        }
    }

    void abs_subtract_small_from_big(const BigInteger& small, bool fl = false) {
        size_t len_small = small.size();
        size_t len_big = a.size();
        if (fl) std::swap(len_small, len_big);
        a.resize(len_big, 0);
        long long sign_fl = (fl ? -1 : 1);
        for (size_t i = 0; i < small.size(); i++) {
            a[i] -= small[i];
            a[i] *= sign_fl;
        }
        long long subtract = 0;
        for (size_t i = 0; i < len_big; i++) {
            a[i] -= subtract;
            subtract = 0;
            if (a[i] < 0) {
                a[i] += base;
                subtract = 1;
            }
        }
        delete_trailing_zeroes(a);
    }

    static bool less_abs(const BigInteger& lhs, const BigInteger& rhs) {
        if (lhs.size() != rhs.size()) return (lhs.size() < rhs.size());
        for (int i = lhs.size() - 1; i >= 0; i--) {
            if (lhs[i] != rhs[i]) return (lhs[i] < rhs[i]);
        }
        return false;
    }

    static void div10(BigInteger& x) {
        long long add = 0;
        for (int i = x.size() - 1; i >= 0; i--) {
            long long cur = x[i] + add * base;
            x[i] = cur / 10;
            add = cur % 10;
        }
        delete_trailing_zeroes(x.a);
    }

    void add_with_sign(const BigInteger& x, Sign mul_sign) {
        Sign lhs_sign = sign;
        Sign rhs_sign = x.sign * mul_sign;
        if (lhs_sign == rhs_sign) {
            long long add = 0;
            size_t len = std::max(a.size(), x.size());
            a.resize(len, 0);
            for (size_t i = 0; i < len; i++) {
                a[i] = a[i] + (i < x.size() ? x[i] : 0) + add;
                add = a[i] / base;
                a[i] %= base;
            }
            a.push_back(add);
            sign = lhs_sign;
        } else {
            if (less_abs(*this, x)) {
                abs_subtract_small_from_big(x, true);
                sign = rhs_sign;
            } else {
                abs_subtract_small_from_big(x, false);
                sign = lhs_sign;
            }
            if (is_zero()) sign = Sign::plus;
        }
        delete_trailing_zeroes(a);
    }

public:
    BigInteger() : a({0}), sign(Sign::plus) {}

    BigInteger(const std::string& s) {
        a.clear();
        sign = Sign::plus;
        int cur = 0;
        if (s[cur] == '-') {
            sign = Sign::minus;
            cur++;
        }
        for (int i = s.size() - 1; i >= cur; i -= base_len) {
            long long number = 0;
            for (int j = std::max(cur, static_cast<int>(i - base_len + 1)); j <= i; j++) {
                number *= 10;
                number += s[j] - '0';
            }
            a.push_back(number);
        }
        delete_trailing_zeroes(a);
    }

    BigInteger(int x) : sign(sgn(x)) {
        x = abs(x);
        do {
            a.push_back(x % base);
            x /= base;
        } while (x > 0);
    }

    BigInteger(const BigInteger& x) : a(x.a), sign(x.sign) {}

    BigInteger& operator=(const BigInteger& x) {
        a = x.a;
        sign = x.sign;
        return *this;
    }

    std::string toString() const {
        std::string res;
        for (size_t i = a.size() - 1; ; i--) {
            res += number_to_string(a[i]);
            if (i == 0) break;
        }
        delete_trailing_zeroes(res);
        if (sign == Sign::minus) res = "-" + res;
        return res;
    }

    BigInteger& operator*=(const BigInteger& x) {
        std::vector<std::complex<double>> fa(a.begin(), a.end());
        std::vector<std::complex<double>> fb(x.a.begin(), x.a.end());
        size_t n = 1;
        while (n < std::max(a.size(), x.a.size())) n <<= 1;
        n <<= 1;
        fa.resize(n);
        fb.resize(n);
        FFT(fa, false);
        FFT(fb, false);
        for (size_t i = 0; i < n; ++i) {
            fa[i] *= fb[i];
        }
        FFT(fa, true);
        std::vector<long long> res(n);
        for (size_t i = 0; i < n; ++i) {
            res[i] = static_cast<long long>(fa[i].real() + 0.5);
        }
        long long add = 0;
        for (size_t i = 0; i < n; i++) {
            res[i] += add;
            add = res[i] / base;
            res[i] %= base;
        }
        delete_trailing_zeroes(res);
        sign = sign * x.sign;
        a = res;
        if (is_zero()) sign = Sign::plus;
        return *this;
    }
    
    BigInteger& operator+=(const BigInteger& x) {
        add_with_sign(x, Sign::plus);
        return *this;
    }

    BigInteger& operator/=(const BigInteger& x) {
        if (x.size() > a.size()) {
            a = {0};
            sign = Sign::plus;
            return *this;
        }
        BigInteger x_abs = x;
        x_abs.sign = Sign::plus;
        std::string divisor = x_abs.toString();
        std::string result;
        sign = sign * x.sign;
        if (sign == Sign::minus) result += '-';
        sign = Sign::plus;
        int degree = (a.size() - x.size() + 1) * base_len;
        divisor += std::string(degree, '0');
        BigInteger sub = divisor;
        for (; degree >= 0; degree--) {
            int number = 0;
            for (; number < 9; number++) {
                if (less_abs(*this, sub)) break;
                *this -= sub;
            }
            result += char('0' + number);
            div10(sub);
        }
        *this = result;
        delete_trailing_zeroes(a);
        if (is_zero()) sign = Sign::plus;
        return *this;
    }

    BigInteger& operator%=(const BigInteger& x) {
        BigInteger copy = *this;
        copy /= x;
        copy *= x;
        *this -= copy;
        if (is_zero()) sign = Sign::plus;
        return *this;
    }

    BigInteger& operator-=(const BigInteger& x) {
        add_with_sign(x, Sign::minus);
        return *this;
    }

    BigInteger& operator++() {
        *this += 1;
        return *this;
    }

    BigInteger operator++(int) {
        BigInteger copy = *this;
        *this += 1;
        return copy;
    }

    BigInteger& operator--() {
        *this -= 1;
        return *this;
    }

    BigInteger operator--(int) {
        BigInteger copy = *this;
        *this -= 1;
        return copy;
    }

    BigInteger operator-() const {
        BigInteger copy = *this;
        if (!copy.is_zero()) copy.sign = copy.sign * Sign::minus;
        return copy;
    }

    explicit operator bool() const {
        return !(is_zero());
    }

    long long& operator[](size_t i) {
        return a[i];
    }

    const long long& operator[](size_t i) const {
        return a[i];
    }

    const Sign& get_sign() const {
        return sign;
    }

    void set_sign(Sign new_sign) {
        sign = new_sign;
    }

    bool is_zero() const {
        return a.size() == 1 && a[0] == 0;
    }

    bool is_one() const {
        return a.size() == 1 && a[0] == 1;
    }

    size_t size() const {
        return a.size();
    }
};

BigInteger gcd(BigInteger a, BigInteger b) {
    while (b) {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

bool operator==(const BigInteger& lhs, const BigInteger& rhs) {
    if (lhs.get_sign() != rhs.get_sign()) return false;
    if (lhs.size() != rhs.size()) return false;
    for (size_t i = 0; i < lhs.size(); i++) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

bool operator!=(const BigInteger& lhs, const BigInteger& rhs) {
    return !(lhs == rhs);
}

bool operator<(const BigInteger& lhs, const BigInteger& rhs) {
    if (lhs.get_sign() != rhs.get_sign()) return lhs.get_sign() < rhs.get_sign();
    if (lhs.size() != rhs.size()) return (lhs.get_sign() == Sign::minus) ^ (lhs.size() < rhs.size());
    for (int i = lhs.size() - 1; i >= 0; i--) {
        if (lhs[i] != rhs[i]) return (lhs.get_sign() == Sign::minus) ^ (lhs[i] < rhs[i]);
    }
    return false;
}

bool operator>(const BigInteger& lhs, const BigInteger& rhs) {
    return rhs < lhs;
}

bool operator<=(const BigInteger& lhs, const BigInteger& rhs) {
    return !(rhs < lhs);
}

bool operator>=(const BigInteger& lhs, const BigInteger& rhs) {
    return !(lhs < rhs);
}

BigInteger operator*(const BigInteger& lhs, const BigInteger& rhs) {
    BigInteger copy = lhs;
    copy *= rhs;
    return copy;
}

BigInteger operator+(const BigInteger& lhs, const BigInteger& rhs) {
    BigInteger copy = lhs;
    copy += rhs;
    return copy;
}

BigInteger operator-(const BigInteger& lhs, const BigInteger& rhs) {
    BigInteger copy = lhs;
    copy -= rhs;
    return copy;
}

BigInteger operator/(const BigInteger& lhs, const BigInteger& rhs) {
    BigInteger copy = lhs;
    copy /= rhs;
    return copy;
}

BigInteger operator%(const BigInteger& lhs, const BigInteger& rhs) {
    BigInteger copy = lhs;
    copy %= rhs;
    return copy;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& a) {
    out << a.toString();
    return out;
}

std::istream& operator>>(std::istream& in, BigInteger& a) {
    std::string s;
    in >> s;
    a = s;
    return in;
}

class Rational {
private:
    BigInteger numerator;
    BigInteger denominator;
    Sign sign = Sign::plus;

private:
    bool is_zero() const {
        return numerator.is_zero();
    }

    void normalize() {
        sign = sign * numerator.get_sign();
        numerator.set_sign(Sign::plus);
        if (is_zero()) sign = Sign::plus;
        BigInteger g = gcd(numerator, denominator);
        if (g.is_one()) return;
        numerator /= g;
        denominator /= g;
    }

public:
    Rational() : numerator(0), denominator(1), sign(Sign::plus) {}

    Rational(int x) : numerator(x), denominator(1), sign(Sign::plus) {
        sign = numerator.get_sign();
        numerator.set_sign(Sign::plus);
    }

    Rational(const BigInteger& x) : numerator(x), denominator(1), sign(Sign::plus) {
        sign = numerator.get_sign();
        numerator.set_sign(Sign::plus);
    }

    Rational(const Rational& x) : numerator(x.numerator), denominator(x.denominator), sign(x.sign) {}

    Rational& operator=(const Rational& x) {
        numerator = x.numerator;
        denominator = x.denominator;
        sign = x.sign;
        return *this;
    }

    std::string toString() const {
        std::string res;
        if (sign == Sign::minus) res += '-';
        res += numerator.toString();
        if (!denominator.is_one()) {
            res += '/';
            res += denominator.toString();
        }
        return res;
    }

    Rational& operator*=(const Rational& x) {
        numerator *= x.numerator;
        denominator *= x.denominator;
        sign = sign * x.sign;
        normalize();
        return *this;
    }

    Rational& operator+=(const Rational& x) {
        numerator.set_sign(sign);
        sign = Sign::plus;
        numerator *= x.denominator;
        if (x.sign == Sign::plus) numerator += x.numerator * denominator;
        else numerator -= x.numerator * denominator;
        denominator *= x.denominator;
        normalize();
        return *this;
    }

    Rational& operator-=(const Rational& x) {
        numerator.set_sign(sign);
        sign = Sign::plus;
        numerator *= x.denominator;
        if (x.sign == Sign::plus) numerator -= x.numerator * denominator;
        else numerator += x.numerator * denominator;
        denominator *= x.denominator;
        normalize();
        return *this;
    }

    Rational& operator/=(const Rational& x) {
        numerator *= x.denominator;
        denominator *= x.numerator;
        sign = sign * x.sign;
        normalize();
        return *this;
    }

    Rational operator-() const {
        Rational copy = *this;
        copy.sign = copy.sign * Sign::minus;
        copy.normalize();
        return copy;
    }

    std::string asDecimal(size_t precision = 0) const {
        BigInteger result = numerator * std::string("1" + std::string(precision, '0'));
        result /= denominator;
        std::string decimal = result.toString();
        int pos = std::max(static_cast<int>(0), static_cast<int>(decimal.size() - precision));
        return (sign == Sign::minus ? std::string("-") : std::string(""))
               + (pos == 0 ? std::string("0") : std::string(""))
               + decimal.substr(0, pos) + "." + std::string(precision - (decimal.size() - pos), '0')
               + decimal.substr(pos, precision);
    }

    explicit operator bool() const {
        return !(is_zero());
    }

    explicit operator double() const {
        return stod(asDecimal(30));
    }

    const Sign& get_sign() const {
        return sign;
    }

    const BigInteger& get_numerator() const {
        return numerator;
    }

    const BigInteger& get_denominator() const {
        return denominator;
    }
};

bool operator==(const Rational& lhs, const Rational& rhs) {
    if (lhs.get_sign() != rhs.get_sign()) return false;
    return lhs.get_numerator() == rhs.get_numerator() && lhs.get_denominator() == rhs.get_denominator();
}

bool operator!=(const Rational& lhs, const Rational& rhs) {
    return !(lhs == rhs);
}

bool operator<(const Rational& lhs, const Rational& rhs) {
    if (lhs.get_sign() != rhs.get_sign()) return lhs.get_sign() < rhs.get_sign();
    return (lhs.get_sign() == Sign::minus) 
            ^ (lhs.get_numerator() * rhs.get_denominator() < rhs.get_numerator() * lhs.get_denominator());
}

bool operator>(const Rational& lhs, const Rational& rhs) {
    return rhs < lhs;
}

bool operator<=(const Rational& lhs, const Rational& rhs) {
    return !(rhs < lhs);
}

bool operator>=(const Rational& lhs, const Rational& rhs) {
    return !(lhs < rhs);
}

Rational operator*(const Rational& lhs, const Rational& rhs) {
    Rational copy = lhs;
    copy *= rhs;
    return copy;
}

Rational operator+(const Rational& lhs, const Rational& rhs) {
    Rational copy = lhs;
    copy += rhs;
    return copy;
}

Rational operator-(const Rational& lhs, const Rational& rhs) {
    Rational copy = lhs;
    copy -= rhs;
    return copy;
}

Rational operator/(const Rational& lhs, const Rational& rhs) {
    Rational copy = lhs;
    copy /= rhs;
    return copy;
}

std::ostream& operator<<(std::ostream& out, const Rational& a) {
    out << a.toString();
    return out;
}

std::istream& operator>>(std::istream& in, Rational& a) {
    std::string s;
    in >> s;
    a = Rational(s);
    return in;
}
}

template<int N, int D, bool fl>
struct IsPrimeHelper {
    static const bool value = N % D != 0 && IsPrimeHelper<N, D + 1, (D + 1) * (D + 1) <= N>::value; 
};

template<int N, int D>
struct IsPrimeHelper<N, D, false> {
    static const bool value = true;
};

template<int N>
struct IsPrime {
    static const bool prime = IsPrimeHelper<N, 2, 2 * 2 <= N>::value;
};

template<>
struct IsPrime<1> {
    static const bool prime = false;
};

template<size_t M, size_t N, typename T>
class Matrix;

template<size_t N>
class Residue {
private:
    size_t value;

private:
    static Residue<N> bin_pow(Residue<N> x, size_t m) {
        Residue<N> result = 1;
        while (m) {
            if (m & 1) {
                result = result * x;
            }
            x = x * x;
            m >>= 1;
        }
        return result;
    }

    static Residue<N> inv(Residue<N> x) {
        return bin_pow(x, N - 2);
    }

public:
    Residue() : value(0) {}

    Residue(int x) : value((x % (int) N + N) % N) {}

    Residue(const Residue<N>& x) : value(x.value) {}

    Residue<N>& operator=(const Residue<N>& x) {
        value = x.value;
        return *this;
    }

    Residue<N>& operator=(int x) {
        Residue<N> copy = x;
        std::swap(*this, copy);
        return *this;
    }

    Residue<N>& operator+=(const Residue<N>& x) {
        value = (value + x.value) % N;
        return *this;
    }

    Residue<N>& operator-=(const Residue<N>& x) {
        value = (value - x.value + N) % N;
        return *this;
    }

    Residue<N>& operator*=(const Residue<N>& x) {
        value = static_cast<long long>(value * x.value) % N;
        return *this;
    }

    Residue<N>& operator/=(const Residue<N>& x) {
        static_assert(IsPrime<N>::prime);
        value = static_cast<long long>(value * inv(x).value) % N;
        return *this;
    }

    Residue<N>& operator++() {
        *this += 1;
        return *this;
    }

    Residue<N> operator++(int) {
        Residue<N> copy = *this;
        *this += 1;
        return copy;
    }

    Residue<N>& operator--() {
        *this -= 1;
        return *this;
    }

    Residue<N> operator--(int) {
        Residue<N> copy = *this;
        *this -= 1;
        return copy;
    }

    Residue<N> operator-() const {
        Residue<N> copy = *this;
        copy *= -1;
        return copy;
    }

    explicit operator int() const {
        return value;
    }

    explicit operator bool() const {
        return value != 0;
    }

    const size_t& get_value() const {
        return value;
    }
    
    bool operator==(const Residue<N>& rhs) const {
        return get_value() == rhs.get_value();
    }

    bool operator!=(const Residue<N>& rhs) const {
        return get_value() != rhs.get_value();
    }
};

template<size_t N>
Residue<N> operator+(const Residue<N>& lhs, const Residue<N>& rhs) {
    Residue<N> copy = lhs;
    copy += rhs;
    return copy;
}

template<size_t N>
Residue<N> operator-(const Residue<N>& lhs, const Residue<N>& rhs) {
    Residue<N> copy = lhs;
    copy -= rhs;
    return copy;
}

template<size_t N>
Residue<N> operator*(const Residue<N>& lhs, const Residue<N>& rhs) {
    Residue<N> copy = lhs;
    copy *= rhs;
    return copy;
}

template<size_t N>
Residue<N> operator/(const Residue<N>& lhs, const Residue<N>& rhs) {
    Residue<N> copy = lhs;
    copy /= rhs;
    return copy;
}

template<size_t N, size_t M, size_t K>
Matrix<M, K, Residue<N>> operator*(const Residue<N>& number, const Matrix<M, K, Residue<N>>& rhs) {
    Matrix<M, K, Residue<N>> result = rhs;
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < K; j++) {
            result[i][j] *= number;
        }
    }
    return result;
}

template<size_t N>
std::ostream& operator<<(std::ostream& out, const Residue<N>& a) {
    out << a.get_value();
    return out;
}

template<size_t N>
std::istream& operator>>(std::istream& in, Residue<N>& a) {
    int x;
    in >> x;
    a = x;
    return in;
}

namespace Row {

template<size_t N, typename Field = BigNumber::Rational>
class Row {
private:
    std::vector<Field> a = std::vector<Field>(N);

public:
    Field& operator[](size_t i) {
        return a[i];
    }

    const Field& operator[](size_t i) const {
        return a[i];
    }

    std::vector<Field> getRow() const {
        return a;
    }
};
}

template<size_t M, size_t N, typename Field = BigNumber::Rational> 
class Matrix {
private:
    std::vector<Row::Row<N, Field>> a = std::vector<Row::Row<N, Field>>(M);

private:
    template<size_t K>
    static size_t get_first_nonzero_pos(Row::Row<K, Field>& a) {
        for (size_t i = 0; i < N; i++) {
            if (a[i]) return i;
        }
        return N;
    }

    template<size_t K>
    static bool is_zero(Row::Row<K, Field>& a) {
        return get_first_nonzero_pos(a) == K;
    }

    template<size_t K>
    static void subtract_row(Row::Row<K, Field>& a, const Row::Row<K, Field>& b, const Field mul_number) {
        for (size_t i = 0; i < K; i++) {
            a[i] -= b[i] * mul_number;
        }
    }

    template<size_t K>
    static void div_row(Row::Row<K, Field>& a, const Field div_number) {
        for (size_t i = 0; i < K; i++) {
            a[i] /= div_number;
        }
    }

public:
    Matrix() {
        if (M == N) {
            for (size_t i = 0; i < M; i++) {
                a[i][i] = 1;
            }
        }
    }

    Matrix(const std::initializer_list<std::initializer_list<int>> x) {
        size_t cur = 0;
        for (auto i = x.begin(); i != x.end(); i++) {
            for (auto j = i->begin(); j != i->end(); j++, cur++) {
                a[cur / N][cur % N] = *j;
            }
        }
    }

    Matrix(const Matrix<M, N, Field>& x) {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] = x[i][j];
            }
        }
    }

    template<typename T>
    Matrix(const std::vector<std::vector<T>>& x) {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] = x[i][j];
            }
        }
    }

    void set_zero() {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] = 0;
            }
        }
    }

    Row::Row<N, Field>& operator[](size_t i) {
        return a[i];
    }

    const Row::Row<N, Field>& operator[](size_t i) const {
        return a[i];
    }

    std::vector<Field> getRow(size_t i) const{
        return a[i].getRow();
    }

    std::vector<Field> getColumn(size_t j) const {
        std::vector<Field> result(M);
        for (size_t i = 0; i < M; i++) {
            result[i] = a[i][j];
        }
        return result;
    }

    Matrix<M, N, Field>& operator+=(const Matrix<M, N, Field>& rhs) {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] += rhs[i][j];
            }
        }
        return *this;
    }

    Matrix<M, N, Field>& operator-=(const Matrix<M, N, Field>& rhs) {
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] -= rhs[i][j];
            }
        }
        return *this;
    }

    Matrix<M, M, Field>& operator*=(const Matrix<M, M, Field>& rhs) {
        static_assert(M == N);
        Matrix<M, M, Field> result;
        result.set_zero();
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < M; j++) {
                for (size_t t = 0; t < M; t++) {
                    result[i][t] += a[i][j] * rhs[j][t];
                }
            }
        }
        std::swap(a, result.a);
        return *this;
    }

    Matrix<N, M, Field> transposed() const {
        Matrix<N, M, Field> result;
        result.set_zero();
        for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < M; j++) {
                result[i][j] = a[j][i];
            }
        }
        return result;
    }

    Field transform_to_triangular_matrix() {
        size_t cnt_swaps = 0;
        Field det = 1;
        for (size_t i = 0; i < M; i++) {
            size_t pos = get_first_nonzero_pos(a[i]);
            size_t best_i = i;
            for (size_t j = i + 1; j < M; j++) {
                size_t pos_j = get_first_nonzero_pos(a[j]);
                if (pos_j < pos) {
                    pos = pos_j;
                    best_i = j;
                }
            }
            if (i != best_i) {
                std::swap(a[i], a[best_i]);
                cnt_swaps++;
            }
            if (M == N) det *= a[i][i];
            if (pos >= N) break; 
            div_row(a[i], a[i][pos]);
            for (size_t j = i + 1; j < M; j++) {
                subtract_row(a[j], a[i], a[j][pos]);
            }
        }
        if (cnt_swaps & 1) det *= -1;
        return det;
    }

    Field det() const {
        static_assert(M == N);
        Matrix<M, N, Field> copy = *this;
        Field det = copy.transform_to_triangular_matrix();
        return det;
    }

    size_t rank() const {
        Matrix<M, N, Field> copy = *this;
        copy.transform_to_triangular_matrix();
        size_t rank = 0;
        for (size_t i = 0; i < M && !is_zero(copy[i]); i++, rank++) {}
        return rank;
    }

    Field trace() const {
        static_assert(M == N);
        Field result = 0;
        for (size_t i = 0; i < M; i++) {
            result += a[i][i];
        }
        return result;
    }
    
    void invert() {
        static_assert(M == N);
        Matrix<M, 2 * N, Field> copy;
        copy.set_zero();
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                copy[i][j] = a[i][j];
            }
        }
        for (size_t i = 0; i < M; i++) {
            copy[i][N + i] = 1; 
        }
        copy.transform_to_triangular_matrix();
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < M - 1 - i; j++) {
                subtract_row(copy[j], copy[M - 1 - i], copy[j][M - 1 - i]);
            }
        }
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                a[i][j] = copy[i][N + j];
            }
        }
    }

    Matrix<M, N, Field> inverted() const {
        Matrix<M, N, Field> copy = *this;
        copy.invert();
        return copy;
    }
};

template<size_t M, size_t N, typename Field = BigNumber::Rational>
Matrix<M, N, Field> operator+(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
    Matrix<M, N, Field> copy = lhs;
    copy += rhs;
    return copy;
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
Matrix<M, N, Field> operator-(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
    Matrix<M, N, Field> copy = lhs;
    copy -= rhs;
    return copy;
}


template<size_t M, size_t N, size_t K, typename Field = BigNumber::Rational>
Matrix<M, K, Field> operator*(const Matrix<M, N, Field>& lhs, const Matrix<N, K, Field>& rhs) {
    Matrix<M, K, Field> result;
    result.set_zero();
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            for (size_t t = 0; t < K; t++) {
                result[i][t] += lhs[i][j] * rhs[j][t];
            }
        }
    }
    return result;
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
Matrix<M, N, Field> operator*(const Matrix<M, N, Field>& lhs, const Field& number) {
    Matrix<M, N, Field> result = lhs;
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            result[i][j] *= number;
        }
    }
    return result;
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
bool operator==(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            if (lhs[i][j] != rhs[i][j]) return false;
        }
    }
    return true;
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
bool operator!=(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
    return !(lhs == rhs);
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
std::ostream& operator<<(std::ostream& out, const Matrix<M, N, Field>& a) {
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            out << a[i][j] << ' ';
        }
        out << '\n';
    }
    return out;
}

template<size_t M, size_t N, typename Field = BigNumber::Rational>
std::istream& operator>>(std::istream& in, Matrix<M, N, Field>& a) {
    for (size_t i = 0; i < M; i++) {
        for (size_t j = 0; j < N; j++) {
            in >> a[i][j];
        }
    }
    return in;
}

template<size_t N, typename Field = BigNumber::Rational>
using SquareMatrix = Matrix<N, N, Field>;