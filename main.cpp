#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

namespace fs = std::filesystem;

std::mutex output_mutex;

void search_in_file(const std::string& filepath, const std::string& keyword)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    std::string line;

    while (getline(file, line))
    {
        if (line.find(keyword) != std::string::npos)
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "Found in: " << filepath << std::endl;
            break;
        }
    }
}

void worker(const std::vector<std::string>& files, const std::string& keyword, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        search_in_file(files[i], keyword);
    }
}

int main()
{
    std::string directory;
    std::string keyword;

    std::cout << "Directory: ";
    std::getline(std::cin, directory);

    std::cout << "Keyword: ";
    std::getline(std::cin, keyword);

    std::vector<std::string> files;

    for (const auto& entry :
        fs::recursive_directory_iterator(directory,
            fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file())
        {
            auto ext = entry.path().extension().string();

            if (ext == ".txt" || ext == ".log" || ext == ".cpp")
            {
                files.push_back(entry.path().string());
            }
        }
    }

    int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 4;

    std::vector<std::thread> threads;

    int files_per_thread = files.size() / thread_count;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < thread_count; i++)
    {
        int start = i * files_per_thread;
        int end = (i == thread_count - 1) ? files.size() : start + files_per_thread;

        threads.emplace_back(worker, std::ref(files), std::ref(keyword), start, end);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "\nSearch completed in " << elapsed.count() << " seconds." << std::endl;

    return 0;
}