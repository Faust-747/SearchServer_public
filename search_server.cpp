#include "search_server.h"
#include "log_duration.h"
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {  return c >= '\0' && c < ' ';  });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            std::string s = ("Word "s + word + " is invalid"s);//если сунуть это в invalid_argument, будет отсудствие конструктора
            throw std::invalid_argument(s); //не воспринимает конкатенацию через +
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        std::string s = ("Query word is empty"s);
        throw std::invalid_argument(s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        std::string s = ("Query word "s + text + " is invalid"s);
        throw std::invalid_argument(s);
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}


void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;   //новый контейнер для GetWordFrequencies
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
    int document_id) const 
{
    LOG_DURATION_STREAM("Матчинг документов по запросу: "s + raw_query, std::cout);
    const auto query = ParseQuery(raw_query);

    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::vector<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::vector<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

// Разработайте метод получения частот слов по id документа
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string, double> word_freqs;
    if (documents_.count(document_id) == 0) {
        return word_freqs;
    }
    word_freqs = document_to_word_freqs_.at(document_id);

    return word_freqs;
}

void SearchServer::RemoveDocument(int document_id) 
{
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    auto it0 = std::remove(document_ids_.begin(), document_ids_.end(), document_id);
    document_ids_.erase(it0, document_ids_.end());

    for (auto& element : word_to_document_freqs_)
    {
        auto it = element.second.find(document_id);
        if (it != element.second.end())
        {
            element.second.erase(it);
        }
    }
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document,
    DocumentStatus status, const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::exception& e) {
        std::cout << "Error in adding document "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Results for request: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error is seaching: "s << e.what() << std::endl;
    }
}

int SearchServer::GetDocumentId(int index) const {
    return document_ids_.at(index);
}

std::vector<int> SearchServer::GetAllDocIDs() const
{
    return document_ids_;
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Matching for request: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error in matchig request "s << query << ": "s << e.what() << std::endl;
    }
}