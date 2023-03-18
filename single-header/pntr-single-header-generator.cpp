#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

using Buffer = std::array<char, 1024u>;

inline constexpr std::streamsize c_comment_width = 100;

inline constexpr std::string_view c_pragma_once{"#pragma once"};
inline constexpr std::string_view c_pntr_include{"#include <pntr/"};


inline void
fill_char(std::ofstream & p_out, char const p_char, std::streamsize p_count)
{
  while (p_count-- != 0 && p_out.put(p_char).good())
  {}
}

static bool
process_license(char * p_license_filename, std::ofstream & p_out)
{
  std::ifstream lic(p_license_filename);
  if (!lic.is_open())
  {
    return false;
  }

  fill_char(p_out, '/', c_comment_width);
  p_out << "\n//";
  fill_char(p_out, ' ', c_comment_width - 4);
  p_out << "//\n";

  std::array<char, 1024u> buf;
  while (lic.getline(buf.data(), buf.size()).good() && p_out.good())
  {
    p_out << "//  ";
    p_out.write(buf.data(), lic.gcount() - 1u);
    fill_char(p_out, ' ', c_comment_width - lic.gcount() - 5);
    p_out << "//\n";
  }

  p_out << "//";
  fill_char(p_out, ' ', c_comment_width - 4);
  p_out << "//\n";
  fill_char(p_out, '/', c_comment_width);
  p_out << "\n\n" << std::flush;

  return true;
}


static bool
process_header(std::string_view const p_header_filename, std::ofstream & p_out)
{
  std::ifstream in(p_header_filename.data());
  if (!in.is_open())
  {
    return false;
  }

  std::array<char, 1024u> buf;

  while (in.getline(buf.data(), buf.size()).good())
  {
    std::string_view const line(buf.data(), static_cast<std::string_view::size_type>(in.gcount() - 1));
    if (line.size() >= c_pragma_once.size()
        && c_pragma_once.compare(0u, c_pragma_once.size(), line, 0u, c_pragma_once.size()) == 0)
    {
      break;
    }
  }

  while (in.getline(buf.data(), buf.size()).good())
  {
    std::string_view const line(buf.data(), static_cast<std::string_view::size_type>(in.gcount() - 1));
    if (!line.empty()
        && (line.size() < c_pntr_include.size()
            || c_pntr_include.compare(0u, c_pntr_include.size(), line, 0u, c_pntr_include.size()) != 0))
    {
      std::streamsize const header_size = static_cast<std::streamsize>(p_header_filename.size());
      std::streamsize const front_padding = (c_comment_width - header_size - 4) / 2;
      std::streamsize const back_padding = c_comment_width - header_size - front_padding - 4;
      p_out.put('\n');
      fill_char(p_out, '/', c_comment_width);
      p_out << "\n//";
      fill_char(p_out, ' ', c_comment_width - 4);
      p_out << "//\n//";
      fill_char(p_out, ' ', front_padding);
      p_out << p_header_filename;
      fill_char(p_out, ' ', back_padding);
      p_out << "//\n//";
      fill_char(p_out, ' ', c_comment_width - 4);
      p_out << "//\n";
      fill_char(p_out, '/', c_comment_width);
      p_out << "\n\n" << line << '\n';
      break;
    }
  }

  while (in.getline(buf.data(), buf.size()).good() && p_out.good())
  {
    p_out << std::string_view(buf.data(), static_cast<std::string_view::size_type>(in.gcount() - 1)) << '\n';
  }

  return p_out.good();
}


int
main(int p_arg_count, char * p_arg_array[])
{
  if (p_arg_count < 4)
  {
    std::cout << "Usage: " << p_arg_array[0] << " <output file> <license file> <input file 1> ... <input file n>"
              << std::endl;
    return (p_arg_count == 1 ? EXIT_SUCCESS : EXIT_FAILURE);
  }

  std::ofstream out(p_arg_array[1], std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out.is_open())
  {
    std::cerr << p_arg_array[0] << ": error: unable to open output file '" << p_arg_array[1] << ' ' << std::endl;
    return EXIT_FAILURE;
  }

  if (!process_license(p_arg_array[2], out))
  {
    std::cerr << p_arg_array[0] << ": error: unable to process license file '" << p_arg_array[2] << ' ' << std::endl;
    return EXIT_FAILURE;
  }

  out << c_pragma_once << "\n";

  for (int index = 3; index < p_arg_count; ++index)
  {
    if (!process_header(p_arg_array[index], out))
    {
      std::cerr << p_arg_array[0] << ": error: unable to process header file '" << p_arg_array[index] << ' '
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
