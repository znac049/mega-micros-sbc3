// A test of the C++ support on the board: run it and every line should
// end "ok". It exercises what the compiler needs from the library and
// crt0 - static objects' constructors and destructors, new and delete,
// pure virtual functions, function-local statics - plus templates and
// placement new.

#include <stdio.h>
#include <string.h>
#include <new>

static int failures;

static void check(const char *what, bool ok) {
    printf("%-44s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

// A static object: its constructor must have run before main()
class Greeting {
public:
    Greeting(const char *text) : text_(text), magic_(0x1234) { }
    ~Greeting() { printf("%-44s ok\n", "static destructor run at exit"); }

    const char *text() const { return text_; }
    int magic() const { return magic_; }

private:
    const char *text_;
    int magic_;
};

static Greeting greeting("hello from a static object");

// Virtual functions, through an abstract base class
class Shape {
public:
    virtual ~Shape() { }
    virtual int area() const = 0;
    virtual const char *name() const { return "shape"; }
};

class Rect : public Shape {
public:
    Rect(int w, int h) : w_(w), h_(h) { }
    int area() const override { return w_ * h_; }
    const char *name() const override { return "rect"; }

private:
    int w_, h_;
};

class Square : public Rect {
public:
    explicit Square(int s) : Rect(s, s) { }
    const char *name() const override { return "square"; }
};

static int destroyed;

class Counted {
public:
    Counted() : value(42) { }
    ~Counted() { destroyed++; }
    int value;
};

template <typename T, int N>
class Stack {
public:
    void push(const T &v) { items_[count_++] = v; }
    T pop() { return items_[--count_]; }
    int size() const { return count_; }

private:
    T items_[N];
    int count_ = 0;
};

template <typename T>
T biggest(T a, T b) { return (a > b) ? a : b; }

static int made;

static int &counter() {
    static int count = ++made;      // made once, the first time through
    return count;
}

int main(int argc, char **argv) {
    (void)argc;

    printf("C++ test (%s)\n", argv[0]);

    check("static constructor ran before main()",
          greeting.magic() == 0x1234 && strcmp(greeting.text(), "hello from a static object") == 0);

    Shape *shapes[] = { new Rect(3, 4), new Square(5) };
    check("virtual functions", shapes[0]->area() == 12 && shapes[1]->area() == 25 &&
          strcmp(shapes[1]->name(), "square") == 0);
    for (Shape *s : shapes) {
        delete s;
    }

    Counted *many = new Counted[10];
    check("new[] runs the constructors", many[0].value == 42 && many[9].value == 42);
    delete[] many;
    check("delete[] runs the destructors", destroyed == 10);

    Counted *huge = new (std::nothrow) Counted[0x10000000];
    check("new (std::nothrow) gives nullptr when it fails", huge == nullptr);

    alignas(Counted) unsigned char buffer[sizeof(Counted)];
    Counted *placed = new (buffer) Counted;
    check("placement new", placed->value == 42 && (void *)placed == (void *)buffer);
    placed->~Counted();

    Stack<int, 8> stack;
    stack.push(1);
    stack.push(2);
    check("templates", stack.pop() == 2 && stack.size() == 1 && biggest(3, 7) == 7 &&
          biggest(2.5, 1.5) == 2.5);

    counter();
    counter();
    check("function-local static made once", counter() == 1 && made == 1);

    auto add = [](int a, int b) { return a + b; };
    check("lambdas", add(2, 3) == 5);

    printf("%d failed\n", failures);

    return failures;
}
