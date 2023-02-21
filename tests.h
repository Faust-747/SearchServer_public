#pragma once

#include <string>
#include <vector>
#include "search_server.h"
#include <cassert>

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    //{
    //    SearchServer server;
    //    server.SetStopWords("in the"s); //<<<==========больше нет такой функции==========
    //    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    //    assert(server.FindTopDocuments("in"s).empty());
    //}
}

void TestMinusWords() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 13;
    const std::string content2 = "cat in the country"s;
    const std::vector<int> ratings2 = { 4, 5, 6 };
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    //должен найтись только один документ - второй отсеится минус-словом
    const auto found_docs = server.FindTopDocuments("cat -country"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    const Document doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id);
    //без минуса должны найтись оба документа
    const auto found_docs2 = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs2.size(), 2);
}

void TestMatchDocuments() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    std::tuple<std::vector<std::string>, DocumentStatus> result = server.MatchDocument("cat in the"s, doc_id);
    ASSERT_EQUAL((get<0>(result)).size(), 1);

    result = server.MatchDocument("-cat in the"s, doc_id);
    ASSERT_EQUAL((get<0>(result)).size(), 0);
}

void Test_SortByRelevance_RelCalc_RatingCalc() {
    const int doc_id = 1;
    const std::string content = "cat flying in the space"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 2;
    const std::string content2 = "random cat in the country"s;
    const std::vector<int> ratings2 = { 4, 5, 6 };

    const int doc_id3 = 3;
    const std::string content3 = "old cat in the country"s;
    const std::vector<int> ratings3 = { 1, 3, 5 };

    SearchServer server("hell"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);

    std::vector<Document> search_results = server.FindTopDocuments("old country cat"s);

    double idf_old = log(3.0 / 1);
    double idf_country = log(3.0 / 2);
    double idf_cat = log(3.0 / 3);

    double tf_old_1 = 0.0 / 5;
    double tf_old_2 = 0.0 / 5;
    double tf_old_3 = 1.0 / 5;

    double tf_country_1 = 0.0 / 5;
    double tf_country_2 = 1.0 / 5;
    double tf_country_3 = 1.0 / 5;

    double tf_cat_1 = 1.0 / 5;
    double tf_cat_2 = 1.0 / 5;
    double tf_cat_3 = 1.0 / 5;

    double tf_idf_1 = idf_old * tf_old_1 +
        idf_country * tf_country_1 +
        idf_cat * tf_cat_1;

    double tf_idf_2 = idf_old * tf_old_2 +
        idf_country * tf_country_2 +
        idf_cat * tf_cat_2;

    double tf_idf_3 = idf_old * tf_old_3 +
        idf_country * tf_country_3 +
        idf_cat * tf_cat_3;

    //тестовый вывод на всякий случай
    std::cout << tf_idf_1 << std::endl
        << tf_idf_2 << std::endl
        << tf_idf_3 << std::endl
        << search_results[2].relevance << std::endl
        << search_results[1].relevance << std::endl
        << search_results[0].relevance << std::endl;

    assert(search_results[0].id == 3 && search_results[1].id == 2 && search_results[2].id == 1);
    assert(search_results[0].rating == 3 && search_results[1].rating == 5 && search_results[2].rating == 2);
    assert(search_results[0].relevance > search_results[1].relevance && search_results[1].relevance > search_results[2].relevance);

    assert(tf_idf_1 == search_results[2].relevance);
    assert(tf_idf_2 == search_results[1].relevance);
    assert(tf_idf_3 == search_results[0].relevance);
}

void TestStatusFiltering() {
    SearchServer server("in the"s);
    server.AddDocument(1, "word"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "word"s, DocumentStatus::BANNED, { 1,2,3 });
    server.AddDocument(3, "word"s, DocumentStatus::IRRELEVANT, { 1,2,3 });
    server.AddDocument(4, "word"s, DocumentStatus::REMOVED, { 1,2,3 });

    const auto search_result1 = server.FindTopDocuments("word"s, DocumentStatus::ACTUAL);
    const auto search_result2 = server.FindTopDocuments("word"s, DocumentStatus::BANNED);
    const auto search_result3 = server.FindTopDocuments("word"s, DocumentStatus::IRRELEVANT);
    const auto search_result4 = server.FindTopDocuments("word"s, DocumentStatus::REMOVED);

    assert(search_result1.size() == 1 and search_result1[0].id == 1);
    assert(search_result2.size() == 1 and search_result2[0].id == 2);
    assert(search_result3.size() == 1 and search_result3[0].id == 3);
    assert(search_result4.size() == 1 and search_result4[0].id == 4);
}

void TestPredicateFiltering() {
    SearchServer server("in the"s);
    server.AddDocument(1, "word"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "word"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(3, "word"s, DocumentStatus::ACTUAL, { 3,4,5 });
    server.AddDocument(4, "word"s, DocumentStatus::ACTUAL, { 4,5,6 });

    const auto search_result1 = server.FindTopDocuments("word"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 1; });
    const auto search_result2 = server.FindTopDocuments("word"s, [](int document_id, DocumentStatus status, int rating) { return rating == 3; });
    //const auto search_result3 = server.FindTopDocuments("word"s, DocumentStatus::IRRELEVANT);
    //const auto search_result4 = server.FindTopDocuments("word"s, DocumentStatus::REMOVED);

    assert(search_result1.size() == 1 and search_result1[0].id == 1);
    assert(search_result2.size() == 1 and search_result2[0].id == 2);
    //assert(search_result3.size() == 1 and search_result3[0].id == 3);
    //assert(search_result4.size() == 1 and search_result4[0].id == 4);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestMinusWords();
    TestMatchDocuments();
    Test_SortByRelevance_RelCalc_RatingCalc();
    TestStatusFiltering();
    TestPredicateFiltering();
    // Не забудьте вызывать остальные тесты здесь
}