#include <iostream>

template <typename T, int SIZE>
void sort(T (&arr)[SIZE])
{
    for (int i = 0; i < SIZE - 1; ++i)
    {
        for (int j = 0; j < SIZE - i - 1; ++j)
        {
            if (arr[j] > arr[j + 1])
            {
                std::swap(arr[j], arr[j + 1]);
            }
        }
    }
}

int main()
{
    int arr[] = { 5, 4, 3, 2, 1 }; // 改成逆序测试
    sort(arr);                     // ✅ 正常工作

    for (int v : arr)
        std::cout << v << " "; // 输出: 1 2 3 4 5
    return 0;
}
