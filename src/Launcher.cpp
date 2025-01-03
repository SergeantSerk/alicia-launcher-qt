#include "Launcher.hpp"

#include <format>
#include <fstream>
#include <map>
#include <openssl/evp.h>
#include <thread>
#include <vector>

namespace launcher
{

std::map<std::string, std::string> obtainFileInfo()
{
  return {
    {"file1", "ecdc5536f73bdae8816f0ea40726ef5e9b810d914493075903bb90623d97b1d8"},
    {"file2", "67ee5478eaadb034ba59944eb977797b49ca6aa8d3574587f36ebcbeeb65f70e"},
    {"file3", "94f6e58bd04a4513b8301e75f40527cf7610c66d1960b26f6ac2e743e108bdac"},
  };
}

std::string sha256_checksum(const std::string& path)
{
  // https://docs.openssl.org/master/man3/EVP_DigestInit#examples
  std::ifstream input(path, std::ios_base::binary);

  if (!input.is_open())
    throw std::logic_error("failed to open file");

  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx)
    throw std::logic_error("message digest creation failed");

  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned md_len;

  const EVP_MD* md = EVP_sha256();
  EVP_DigestInit_ex2(mdctx, md, nullptr);

  do
  {
    unsigned char buffer[1024];
    input.read(reinterpret_cast<char*>(buffer), sizeof(buffer));
    if (input.bad())
    {
      input.close();
      throw std::logic_error("failed to read file");
    }
    if (EVP_DigestUpdate(mdctx, buffer, input.gcount()))
    {
      input.close();
      throw std::logic_error("failed to update digest");
    }

  } while (!input.eof());

  input.close();

  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);

  std::ostringstream ss{};

  for (int i = 0; i < md_len; i++)
  {
    char buffer[3] = {};
    snprintf(buffer, sizeof(buffer), "%02x", md_value[i]);
    ss << buffer;
  }

  return ss.str();
}

Profile authenticate(std::string_view const& username, std::string_view const& password)
{
  std::this_thread::sleep_for(std::chrono::seconds(2));
  return Profile{};
}

std::vector<std::string> fileCheck()
{
  auto expected = obtainFileInfo();
  auto unexpected = std::vector<std::string>();

  for (const auto& [path, expected_sum] : expected)
  {
    try
    {
      if (auto sum = sha256_checksum(path); sum != expected_sum)
      {
        unexpected.push_back(path);
      }
    }
    catch (std::logic_error const& e)
    {
      unexpected.push_back(path);
    }
  }
  return unexpected;
}

bool fileUpdate(std::vector<std::string> const& files) { return false; }

bool launch(Profile const& profile) { return false; }

} // namespace launcher
