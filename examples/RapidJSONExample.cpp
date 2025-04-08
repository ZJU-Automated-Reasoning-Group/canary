#include <iostream>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

int main() {
    // 1. Parse a JSON string into DOM.
    const char* json = R"({"project":"canary","features":["alias analysis","SMT solving","BDD"],"version":1.0})";
    Document document;
    document.Parse(json);

    // 2. Access values in the DOM.
    if (document.HasMember("project") && document["project"].IsString()) {
        std::cout << "Project: " << document["project"].GetString() << std::endl;
    }

    // Access array
    if (document.HasMember("features") && document["features"].IsArray()) {
        const Value& features = document["features"];
        std::cout << "Features: ";
        for (SizeType i = 0; i < features.Size(); i++) {
            std::cout << features[i].GetString();
            if (i < features.Size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }

    // Access number
    if (document.HasMember("version") && document["version"].IsDouble()) {
        std::cout << "Version: " << document["version"].GetDouble() << std::endl;
    }

    // 3. Modify the DOM
    document.AddMember("integrated", true, document.GetAllocator());
    
    // Add new feature to the array
    if (document.HasMember("features") && document["features"].IsArray()) {
        Value& features = document["features"];
        Value newFeature;
        newFeature.SetString("JSON parsing");
        features.PushBack(newFeature, document.GetAllocator());
    }

    // 4. Stringify the DOM
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    document.Accept(writer);

    std::cout << "\nModified JSON: " << buffer.GetString() << std::endl;

    return 0;
} 