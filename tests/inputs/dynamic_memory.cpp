int main() {
    int *p; int *arr;
    p = new int;
    arr = new int[5];
    delete p;
    delete[] arr;
    return 0;
}
