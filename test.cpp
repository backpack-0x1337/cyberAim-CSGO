#include <iostream>
#include <vector>


std::vector<int> foo() {
    std::vector<int> temp;
    int x = 1;
    int y = 2;
    temp.push_back(x);
    temp.push_back(y);
    printf("done\n");
    return temp;
}

int main() {
    std::vector<int> fooList = foo();
    std::cout << fooList[0] << std::endl;
    return 0;
}

