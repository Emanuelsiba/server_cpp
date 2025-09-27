#include <iostream>
#include "server.hpp"
#include <httplib.h>
#include <filesystem>
#include <fstream>



namespace fs = std::filesystem;
using namespace httplib;



std::string list_directory(const fs::path& dir_path, const std::string& url_path = "/") {
    std::string html = "<html><body>";
    html += "<h2>Files at " + dir_path.string() + "</h2>";
    html += "<table border='1' cellpadding='5'><tr><th>Name</th><th>Size</th><th>Modified</th></tr>";


    if (dir_path.has_parent_path()) {
        std::string parent_url = url_path.substr(0, url_path.find_last_of('/', url_path.size() - 2) + 1);
    }

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        std::string name = entry.path().filename().string();


        if (!name.empty() && name[0] == '.') continue;

        std::string href = url_path + name;
        if (fs::is_directory(entry)) href += "/";

        auto ftime = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now()
            + std::chrono::system_clock::now()
        );
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        std::string time_str = std::string(std::ctime(&cftime));
        if (!time_str.empty() && time_str.back() == '\n') time_str.pop_back();

        std::string size_str = "-";
        if (fs::is_regular_file(entry))
            size_str = std::to_string(fs::file_size(entry)) + " bytes";

        html += "<tr><td><a href='" + href + "'>" + name + "</a></td><td>" + size_str + "</td><td>" + time_str + "</td></tr>";
    }

    html += "</table></body></html>";
    return html;
}




void start_server() {

    Server server;

    fs::path base_dir = "/path/";
    // std::cout << "Enter the path\n";
    // std::cin >> base_dir;

    server.Get(R"(/(.*))", [base_dir](const Request& req, Response& res) {

        std::string rel_path = req.matches[1];
        fs::path full_path = base_dir / rel_path;
        full_path = fs::canonical(full_path);

        if (full_path.string().find(base_dir.string()) != 0) {
            res.status = 403;
            res.set_content("Acess denied", "text/plain");
            return;
        }

        if (!fs::exists(full_path)) {
            res.status = 404;
            res.set_content("File or Folder not found", "text/plain");
            return;
        }

        if (fs::is_directory(full_path)) {
            res.set_content(list_directory(full_path, "/" + rel_path + (rel_path.empty() ? "" : "/")), "text/html; charset=utf-8");
        } else if (fs::is_regular_file(full_path)) {

            std::ifstream ifs(full_path, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(ifs)), {});
            res.set_content(content, "application/octet-stream");
        }
    });

    std::cout << "Server running at http://localhost:8080" << std::endl;
    server.listen("0.0.0.0", 8080);

}
