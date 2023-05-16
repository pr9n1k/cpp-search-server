#pragma once
#include "document.h"
#include <iostream>
#include <vector>

std::string ReadLine();
int ReadLineWithNumber();
std::ostream& operator<<(std::ostream& out, const Document& document);
std::ostream& operator<<(std::ostream& out, const DocumentStatus& status);

template<typename T>
void Print(std::ostream& out, const T& container) {
    bool firstStep = true;

    for (const auto& element : container) {
        if (!firstStep) {
            out << ", ";
        }
        out << element;
        firstStep = false;
    }
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container) {
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}
