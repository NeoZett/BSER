// New example program

#include <bservm.hpp>
#include <iostream>

int main()
{
    bservm::Program program;
    program
        .print("This library uses the BSER serializer, made by Neo Zetterberg\n")
        .print("to serialize and deserialize programs that can be run using\n")
        .print("the C++ virtual machine. The BSER virtual machine can be used\n")
        .print("to create and run programs in C++.\n\n")
        .print("Ever since version 1.0.3 the new virtual machine requires C++17\n")
        .print("or higher. The primary part of this library is the serializer,\n")
        .print("and therefore we don't prioritise the virtual machine. It is more\n")
        .print("of an extension and optional tool made for those who wants to\n")
        .print("explore the possibilities of this BSER library.\n\n")
        .print("Please contribute to us here: https://github.com/NeoZett/BSER\n")
        .print("Thank you for your contributions and your time!\n\n");

    program.write(L"info.bser");

    bservm::VirtualMachine vm;

    vm.load(L"info.bser");
    vm.execute();

    return 0;
}