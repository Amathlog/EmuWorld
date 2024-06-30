#pragma once

#include <common/utils/visitor.h>
#include <fstream>
#include <string>

namespace Common
{
namespace Utils
{

enum class FileVersion : int32_t
{
    NoVersion = 0,
    Version_0_0_1 = 1,

    CurrentVersion = Version_0_0_1,
};

class FileReadVisitor : public IReadVisitor
{
public:
    FileReadVisitor(const std::string& file, bool withVersioning = false, bool binary = true);
    ~FileReadVisitor();

    void Read(uint8_t* data, size_t size) override;
    void Peek(uint8_t* data, size_t size) override;
    size_t Remaining() const override;
    void Advance(size_t size) override;

    bool IsValid() const { return m_file.is_open(); }
    FileVersion GetVersion() const { return m_version; }

private:
    std::ifstream m_file;
    size_t m_ptr;
    size_t m_size;
    FileVersion m_version;
};

class FileWriteVisitor : public IWriteVisitor
{
public:
    FileWriteVisitor(const std::string& file, bool withVersioning = false, bool binary = true);
    ~FileWriteVisitor();

    void Write(const uint8_t* data, size_t size) override;
    size_t Written() const override;

    bool IsValid() const { return m_file.is_open(); }
    FileVersion GetVersion() const { return m_version; }

private:
    std::ofstream m_file;
    size_t m_ptr;
    FileVersion m_version;
};

} // namespace Utils
} // namespace Common