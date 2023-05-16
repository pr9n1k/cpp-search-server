#pragma once
#include <iostream>
#include <vector>
#include "search_server.h"
#include "read_input_functions.h"

const double ACCURACY = 1e-6;

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        std::abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T& func, const std::string& name_func) {
    func();
    std::cerr << name_func << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func) 

// Конец реализации макросов

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();
//Тест на корректный поиск документов
void TestCorrectSearchedDocs();
//Документы содержащие минус слова не должны включаться в результат поиска
void TestCorrectReturnQueryWithMinusWords();
void TestMatchingDocs();
void TestSortedFindDocs();
void TestComputeAverageRating();
void TestFilterTopDocsWithPredicate();
void TestFindDocsWithStatus();
void TestRelevanceTopDocs();
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();