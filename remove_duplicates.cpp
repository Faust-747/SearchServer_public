#include "remove_duplicates.h"
#include <iterator>

void RemoveDuplicates(SearchServer& search_server)
{
    //создаём локальную базу данных
    std::vector <std::pair<int, std::vector<std::string>>> id_to_words;
    for (int id : search_server.GetAllDocIDs())
    {
        std::vector<std::string> words;     //оставляем только слова, отсекаем частоты
        for (const auto& element : search_server.GetWordFrequencies(id))
        {
            words.push_back(element.first);
        }
        id_to_words.push_back({ id, words });
    }


    for (auto i = 0; i < id_to_words.size() - 1; ++i)
    {
        for (auto j = i + 1; j < id_to_words.size(); ++j)
        {
            if (id_to_words[i].second == id_to_words[j].second)
            {
                std::cout << "Found duplicate document id " << id_to_words[j].first << std::endl;
                search_server.RemoveDocument(id_to_words[j].first);
                id_to_words.erase(id_to_words.begin() + j);
                --j;
            }
        }
    }
}