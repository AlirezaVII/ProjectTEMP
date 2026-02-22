#ifndef DOTENV_H
#define DOTENV_H

#include <string>

/**
 * Reads a .env file and sets the key-value pairs as environment variables.
 * Key-value pairs can then be accessed using std::getenv("KEY_NAME").
 * * @param filename The path to the .env file (default is ".env")
 */
void load_dotenv(const std::string& filename = ".env");

#endif