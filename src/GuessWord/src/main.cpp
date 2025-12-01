#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <ranges>
#include <format>
#include <concepts>
#include <array>

/// 使用C++20的模块化导入（如果编译器支持）
// import <iostream>;
// import <string>;
// import <vector>;
// import <algorithm>;

/// 简化命名空间使用
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

/// C++20 概念约束
template <typename T>
concept StringContainer = requires(T t) {
    { t.begin() } -> std::input_iterator;
    { t.end() } -> std::input_iterator;
    requires std::same_as<std::decay_t<decltype(*t.begin())>, char>;
};

class WordGame
{
private:
    static constexpr int MAX_WRONG = 8;

    vector<string> words = { "GUESS", "CPLUSPLUS", "HANGMAN", "PROGRAM", "DEVELOPER" };
    string         targetWord;
    string         currentGuess;
    string         usedLetters;
    int            wrongAttempts = 0;

    std::mt19937 rng{ std::random_device{}() };

public:
    WordGame()
    {
        initializeGame();
    }

    void initializeGame()
    {
        std::ranges::shuffle(words, rng);
        targetWord   = words.front();
        currentGuess = string(targetWord.size(), '-');
        usedLetters.clear();
        wrongAttempts = 0;
    }

    void displayGameState() const
    {
        cout << std::format("\n=== 猜单词游戏 ===\n");
        cout << std::format("剩余尝试次数: {}\n", MAX_WRONG - wrongAttempts);
        cout << std::format("已用字母: {}\n", usedLetters.empty() ? "无" : usedLetters);
        cout << std::format("当前单词: {}\n", currentGuess);
    }

    [[nodiscard]] char getValidGuess() const
    {
        char guess;
        while (true)
        {
            cout << "请输入一个字母: ";
            if (!(cin >> guess))
            {
                cin.clear();
                cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                cout << "输入无效，请重新输入！\n";
                continue;
            }

            guess = std::toupper(guess);

            if (!std::isalpha(guess))
            {
                cout << "请输入有效的字母！\n";
                continue;
            }

            if (usedLetters.contains(guess))
            { /// C++23 或 C++20 的 string::contains
                cout << std::format("字母 '{}' 已经使用过了！\n", guess);
                continue;
            }

            return guess;
        }
    }

    void processGuess(char guess)
    {
        usedLetters += guess;

        bool found = false;
        for (size_t i = 0; i < targetWord.size(); ++i)
        {
            if (targetWord[i] == guess)
            {
                currentGuess[i] = guess;
                found           = true;
            }
        }

        if (found)
        {
            cout << std::format("恭喜！字母 '{}' 在单词中！\n", guess);
        }
        else
        {
            ++wrongAttempts;
            cout << std::format("抱歉，字母 '{}' 不在单词中。\n", guess);
            displayHangman();
        }
    }

    void displayHangman() const
    {
        constexpr std::array<const char*, 9> hangmanStates = {
            "  +---+\n      |\n      |\n      |\n      |\n      |\n=========",
            "  +---+\n  |   |\n      |\n      |\n      |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n      |\n      |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n  |   |\n      |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n /|   |\n      |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n /|\\  |\n      |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n /|\\  |\n /    |\n      |\n=========",
            "  +---+\n  |   |\n  O   |\n /|\\  |\n / \\  |\n      |\n=========",
            "  +---+\n  |   |\n [O]  |\n /|\\  |\n / \\  |\n      |\n========="
        };

        if (wrongAttempts > 0 && wrongAttempts <= MAX_WRONG)
        {
            cout << hangmanStates[wrongAttempts] << "\n";
        }
    }

    bool isGameOver() const
    {
        return wrongAttempts >= MAX_WRONG || currentGuess == targetWord;
    }

    void displayResult() const
    {
        cout << "\n" << std::string(40, '=') << "\n";

        if (wrongAttempts >= MAX_WRONG)
        {
            cout << std::format("游戏结束！您已用尽所有尝试次数。\n");
            cout << std::format("正确答案是: {}\n", targetWord);
        }
        else
        {
            cout << std::format("恭喜您！猜对了单词！\n");
            cout << std::format("单词是: {}\n", targetWord);
            cout << std::format("您使用了 {} 次尝试。\n", usedLetters.size() + wrongAttempts);
        }

        auto stats = getGameStats();
        cout << std::format("\n游戏统计:\n");
        cout << std::format("  正确字母: {}\n", std::get<0>(stats));
        cout << std::format("  错误字母: {}\n", std::get<1>(stats));
        cout << std::format("  总尝试次数: {}\n", std::get<2>(stats));
    }

    auto getGameStats() const
    {
        int correct =
                std::ranges::count_if(usedLetters,
                                      [this](char c)
                                      {
                                          return targetWord.contains(c); // C++23，或 targetWord.find(c) != string::npos
                                      });
        int incorrect = usedLetters.size() - correct;
        int total     = correct + incorrect + wrongAttempts;
        return std::make_tuple(correct, incorrect, total);
    }

    void run()
    {
        displayInstructions();

        while (!isGameOver())
        {
            displayGameState();
            char guess = getValidGuess();
            processGuess(guess);
        }

        displayResult();
    }

private:
    void displayInstructions() const
    {
        /// 使用原始字符串字面量简化多行文本
        constexpr auto instructions = R"(
========================================
           猜单词游戏
========================================
游戏规则:
1. 系统会随机选择一个英文单词
2. 单词会以 '-' 形式显示
3. 您每次可以猜一个字母
4. 如果字母在单词中，会显示其位置
5. 您最多有 8 次猜错的机会
6. 猜出完整单词或错误次数用尽游戏结束

示例:
单词: APPLE
显示: -----
输入: P
显示: -PP--
========================================
)";

        cout << instructions;
    }
};

int main()
{
    try
    {
        WordGame game;
        game.run();

        cout << "\n是否再来一局? (Y/N): ";
        char choice;
        cin >> choice;

        if (std::toupper(choice) == 'Y')
        {
            WordGame newGame;
            newGame.run();
        }
    }
    catch (const std::exception& e)
    {
        cout << std::format("游戏出错: {}\n", e.what());
        return 1;
    }

    cout << "\n感谢游玩！再见！\n";


    cout << "按 Enter 键退出...";
    cin.ignore();
    cin.get();

    return 0;
}
