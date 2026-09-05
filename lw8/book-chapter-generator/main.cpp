#include <generator>
#include <iostream>
#include <string>
#include <vector>

struct Book
{
	std::string title;
	std::string author;
	std::vector<std::string> chapters;
};

struct BookChapter
{
	std::string bookTitle;
	std::string bookAuthor;
	std::string chapterTitle;
};

std::ostream& operator<<(std::ostream& os, const BookChapter& chapter)
{
	os << "[" << chapter.bookAuthor << "] "
	   << chapter.bookTitle << ": "
	   << chapter.chapterTitle;
	return os;
}

std::generator<BookChapter> ListBookChapters(const std::vector<Book>& books)
{
	for (const auto& book : books)
	{
		for (const auto& chapterTitle : book.chapters)
		{
			co_yield BookChapter{
				.bookTitle = book.title,
				.bookAuthor = book.author,
				.chapterTitle = chapterTitle
			};	
		}
	}
}

int main()
{
	try
	{
		const std::vector<Book> books = {
			{ "The Great Gatsby", "F. Scott Fitzgerald",
				{ "Chapter 1", "Chapter 2" } },
			{ "1984", "George Orwell",
				{ "Chapter 1", "Chapter 2", "Chapter 3" } },
			{ "To Kill a Mockingbird", "Harper Lee",
				{ "Chapter 1" } }
		};
		for (const auto& chapter : ListBookChapters(books))
		{
			if (!(std::cout << chapter << std::endl))
			{
				throw std::runtime_error("Failed to write to stdout");
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}