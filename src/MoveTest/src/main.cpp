#include <iostream>

class Object
{
public:
    Object() : m_ptr(new int(10))
    {
        std::cout << "Object created\n";
    }
    ~Object()
    {
        delete m_ptr;
        std::cout << "Object destroyed\n";
    }

private:
    int *m_ptr = nullptr;
};

Object get_object(bool flag)
{
    Object A;
    Object B;
    if (flag)
    {
        return A;
    }
    else
    {
        return B;
    }
}

int main(int argc, char *argv[])
{
    Object myObj = get_object(true);

    std::cout << "hello world" << std::endl;

    return 0;
}
