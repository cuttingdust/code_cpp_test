#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

/// @brief 玩家类型枚举
enum class Player
{
    NONE,     ///< 无玩家（游戏进行中）
    HUMAN,    ///< 人类玩家
    COMPUTER, ///< 电脑玩家
    TIE       ///< 平局
};

/// @brief 棋子符号枚举
enum class Symbol
{
    X     = 'X', ///< X 棋子
    O     = 'O', ///< O 棋子
    EMPTY = ' '  ///< 空位置
};

/// @brief 游戏常量定义类
class Constants
{
public:
    static constexpr int BOARD_SIZE = 9; ///< 棋盘总格子数（3x3）

    /// @brief 所有获胜组合（8种三连珠情况）
    static constexpr int WINNING_COMBINATIONS[8][3] = {
        { 0, 1, 2 }, ///< 第一行
        { 3, 4, 5 }, ///< 第二行
        { 6, 7, 8 }, ///< 第三行
        { 0, 3, 6 }, ///< 第一列
        { 1, 4, 7 }, ///< 第二列
        { 2, 5, 8 }, ///< 第三列
        { 0, 4, 8 }, ///< 主对角线
        { 2, 4, 6 }  ///< 副对角线
    };

    /// @brief 最佳落子位置顺序（中心优先策略）
    static constexpr int BEST_MOVES[9] = { 4, 0, 2, 6, 8, 1, 3, 5, 7 };
};

/// @brief 游戏棋盘类，封装棋盘状态和操作
class GameBoard
{
private:
    std::vector<char> board_; ///< 棋盘状态数组

public:
    /// @brief 构造函数，初始化空棋盘
    GameBoard() : board_(Constants::BOARD_SIZE, static_cast<char>(Symbol::EMPTY))
    {
    }

    /// @brief 显示当前棋盘状态
    void display() const
    {
        std::cout << "\n";
        for (int i = 0; i < Constants::BOARD_SIZE; ++i)
        {
            /// 如果是空位置显示编号，否则显示棋子
            std::cout << " " << (board_[i] == static_cast<char>(Symbol::EMPTY) ? std::to_string(i)[0] : board_[i]);

            /// 处理行尾和分隔线
            if ((i + 1) % 3 == 0)
            {
                if (i < Constants::BOARD_SIZE - 1)
                {
                    std::cout << "\n---+---+---\n";
                }
            }
            else
            {
                std::cout << " |";
            }
        }
        std::cout << "\n\n";
    }

    /// @brief 显示游戏说明和棋盘布局
    void displayInstructions() const
    {
        std::cout << "\n========== 井字棋游戏 ==========\n";
        std::cout << "棋盘位置编号如下：\n";
        std::cout << " 0 | 1 | 2\n";
        std::cout << "---+---+---\n";
        std::cout << " 3 | 4 | 5\n";
        std::cout << "---+---+---\n";
        std::cout << " 6 | 7 | 8\n";
        std::cout << "===============================\n\n";
    }

    /// @brief 检查指定位置是否可以落子
    /// @param position 位置编号（0-8）
    /// @return 如果位置有效且为空则返回true
    bool isValidMove(int position) const
    {
        return position >= 0 && position < Constants::BOARD_SIZE &&
                board_[position] == static_cast<char>(Symbol::EMPTY);
    }

    /// @brief 获取所有有效移动位置
    /// @return 有效位置编号的向量
    std::vector<int> getValidMoves() const
    {
        std::vector<int> moves;
        moves.reserve(Constants::BOARD_SIZE); /// 预分配空间

        for (int i = 0; i < Constants::BOARD_SIZE; ++i)
        {
            if (isValidMove(i))
            {
                moves.push_back(i);
            }
        }
        return moves;
    }

    /// @brief 在指定位置落子
    /// @param position 位置编号
    /// @param symbol 棋子符号
    /// @return 如果落子成功返回true，否则返回false
    bool placeMove(int position, char symbol)
    {
        if (!isValidMove(position))
        {
            return false;
        }
        board_[position] = symbol;
        return true;
    }

    /// @brief 获取棋盘状态（只读）
    /// @return 棋盘状态向量的常量引用
    const std::vector<char>& getBoard() const
    {
        return board_;
    }

    /// @brief 清空棋盘（重置所有位置为空）
    void clear()
    {
        std::ranges::fill(board_, static_cast<char>(Symbol::EMPTY));
    }

    /// @brief 检查棋盘是否已满
    /// @return 如果所有位置都有棋子则返回true
    bool isFull() const
    {
        return std::ranges::all_of(board_, [](char c) { return c != static_cast<char>(Symbol::EMPTY); });
    }
};

/// @brief 游戏逻辑处理类
class GameLogic
{
public:
    /// @brief 检查当前棋盘状态是否有获胜者
    /// @param board 棋盘状态
    /// @return 获胜者类型（HUMAN/COMPUTER/TIE/NONE）
    static Player checkWinner(const std::vector<char>& board)
    {
        /// 检查所有获胜组合
        for (const auto& combo : Constants::WINNING_COMBINATIONS)
        {
            if (board[combo[0]] != static_cast<char>(Symbol::EMPTY) && board[combo[0]] == board[combo[1]] &&
                board[combo[1]] == board[combo[2]])
            {
                /// 假设X为人类，O为电脑（可根据实际游戏设置调整）
                return (board[combo[0]] == static_cast<char>(Symbol::X)) ? Player::HUMAN : Player::COMPUTER;
            }
        }

        /// 检查是否平局（棋盘已满且无人获胜）
        if (std::ranges::all_of(board, [](const char c) { return c != static_cast<char>(Symbol::EMPTY); }))
        {
            return Player::TIE;
        }

        return Player::NONE;
    }

    /// @brief 获取对手的棋子符号
    /// @param symbol 当前棋子符号
    /// @return 对手的棋子符号
    static char getOpponentSymbol(char symbol)
    {
        return (symbol == static_cast<char>(Symbol::X)) ? static_cast<char>(Symbol::O) : static_cast<char>(Symbol::X);
    }
};

/// @brief 用户输入处理类
class InputHandler
{
public:
    /// @brief 获取指定范围内的数字输入
    /// @param prompt 提示信息
    /// @param min 最小值
    /// @param max 最大值
    /// @return 用户输入的有效数字
    static int getNumberInput(const std::string& prompt, int min, int max)
    {
        int input;
        while (true)
        {
            std::cout << prompt << " [" << min << "-" << max << "]: ";

            if (!(std::cin >> input))
            {
                std::cout << "请输入有效的数字！\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            if (input >= min && input <= max)
            {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return input;
            }

            std::cout << "输入超出范围，请重新输入！\n";
        }
    }

    /// @brief 获取Yes/No选择输入
    /// @param question 提问内容
    /// @return true表示用户选择Yes，false表示No
    static bool getYesNoInput(const std::string& question)
    {
        char response;
        while (true)
        {
            std::cout << question << " (y/n): ";
            std::cin >> response;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            response = static_cast<char>(std::tolower(static_cast<unsigned char>(response)));
            if (response == 'y' || response == 'n')
            {
                return response == 'y';
            }

            std::cout << "请输入 'y' 或 'n'！\n";
        }
    }
};

/// @brief AI玩家类，实现电脑落子逻辑
class AIPlayer
{
private:
    char         aiSymbol_    = ' '; ///< AI使用的棋子符号
    char         humanSymbol_ = ' '; ///< 人类使用的棋子符号
    std::mt19937 rng_;               ///< C++20 随机数生成器

public:
    /// @brief 构造函数
    /// @param symbol AI使用的棋子符号
    /// @param seed 随机数种子（默认使用随机设备）
    AIPlayer(char symbol, unsigned int seed = std::random_device{}()) : aiSymbol_(symbol), rng_(seed)
    {
        humanSymbol_ = GameLogic::getOpponentSymbol(symbol);
    }

    /// @brief 获取最佳落子位置
    /// @param board 当前棋盘状态
    /// @return 建议的落子位置编号，-1表示无有效移动
    int getBestMove(const std::vector<char>& board)
    {
        /// 策略1：检查AI是否能够立即获胜
        int winMove = findWinningMove(board, aiSymbol_);
        if (winMove != -1)
        {
            std::cout << "AI选择获胜位置: " << winMove << "\n";
            return winMove;
        }

        /// 策略2：阻止人类玩家获胜
        int blockMove = findWinningMove(board, humanSymbol_);
        if (blockMove != -1)
        {
            std::cout << "AI选择阻止位置: " << blockMove << "\n";
            return blockMove;
        }

        ///  策略3：按照最佳位置顺序选择
        for (int move : Constants::BEST_MOVES)
        {
            if (move >= 0 && move < Constants::BOARD_SIZE && board[move] == static_cast<char>(Symbol::EMPTY))
            {
                std::cout << "AI选择策略位置: " << move << "\n";
                return move;
            }
        }

        /// 策略4：随机选择一个有效位置
        return getRandomMove(board);
    }

private:
    /// @brief 寻找能够立即获胜的落子位置
    /// @param board 当前棋盘状态
    /// @param symbol 要检查的棋子符号
    /// @return 获胜位置编号，-1表示没有立即获胜的位置
    int findWinningMove(const std::vector<char>& board, char symbol)
    {
        for (int i = 0; i < Constants::BOARD_SIZE; ++i)
        {
            if (board[i] == static_cast<char>(Symbol::EMPTY))
            {
                /// 模拟在这个位置落子
                std::vector<char> testBoard = board;
                testBoard[i]                = symbol;

                /// 检查是否获胜
                Player result = GameLogic::checkWinner(testBoard);

                if (bool isWinner = (symbol == aiSymbol_) ? (result == Player::COMPUTER) : (result == Player::HUMAN))
                {
                    return i;
                }
            }
        }
        return -1;
    }

    /// @brief 随机选择一个有效落子位置
    /// @param board 当前棋盘状态
    /// @return 随机选择的落子位置，-1表示无有效移动
    int getRandomMove(const std::vector<char>& board)
    {
        std::vector<int> validMoves = getValidMovesList(board);

        if (!validMoves.empty())
        {
            /// 使用C++20的均匀分布生成随机索引
            std::uniform_int_distribution<std::size_t> dist(0, validMoves.size() - 1);
            return validMoves[dist(rng_)];
        }

        return -1;
    }

    /// @brief 获取所有有效移动位置的列表
    /// @param board 棋盘状态
    /// @return 有效位置编号的向量
    std::vector<int> getValidMovesList(const std::vector<char>& board) const
    {
        std::vector<int> moves;
        moves.reserve(Constants::BOARD_SIZE);

        for (int i = 0; i < Constants::BOARD_SIZE; ++i)
        {
            if (board[i] == static_cast<char>(Symbol::EMPTY))
            {
                moves.push_back(i);
            }
        }

        return moves;
    }
};

/// @brief 井字棋游戏主类，管理游戏流程
class TicTacToeGame
{
private:
    GameBoard board_;                    ///< 游戏棋盘
    char      humanSymbol_    = ' ';     ///< 人类玩家棋子符号
    char      computerSymbol_ = ' ';     ///< 电脑玩家棋子符号
    AIPlayer* aiPlayer_       = nullptr; ///< AI玩家实例
    bool      humanTurnFirst_ = false;   ///< 人类是否先手

public:
    /// @brief 构造函数
    TicTacToeGame() : aiPlayer_(nullptr)
    {
        initialize();
    }

    /// @brief 析构函数
    ~TicTacToeGame()
    {
        delete aiPlayer_;
    }

    /// 禁止拷贝和移动
    TicTacToeGame(const TicTacToeGame&)            = delete;
    TicTacToeGame& operator=(const TicTacToeGame&) = delete;
    TicTacToeGame(TicTacToeGame&&)                 = delete;
    TicTacToeGame& operator=(TicTacToeGame&&)      = delete;

    /// @brief 初始化游戏设置
    void initialize()
    {
        board_.clear();
        board_.displayInstructions();

        /// 选择先手玩家
        humanTurnFirst_ = InputHandler::getYesNoInput("您想要先手吗？");

        if (humanTurnFirst_)
        {
            humanSymbol_    = static_cast<char>(Symbol::X);
            computerSymbol_ = static_cast<char>(Symbol::O);
            std::cout << "\n您将使用 X，电脑使用 O，您先手。\n";
        }
        else
        {
            humanSymbol_    = static_cast<char>(Symbol::O);
            computerSymbol_ = static_cast<char>(Symbol::X);
            std::cout << "\n您将使用 O，电脑使用 X，电脑先手。\n";
        }

        /// 使用随机设备初始化AI的随机数生成器
        aiPlayer_ = new AIPlayer(computerSymbol_);
    }

    /// @brief 运行游戏主循环
    void run()
    {
        bool playAgain = true;

        while (playAgain)
        {
            playGame();
            playAgain = InputHandler::getYesNoInput("\n再来一局？");

            if (playAgain)
            {
                resetGame();
            }
        }

        std::cout << "\n感谢游玩！再见！\n";
    }

private:
    /// @brief 进行一局完整的游戏
    void playGame()
    {
        Player currentPlayer = humanTurnFirst_ ? Player::HUMAN : Player::COMPUTER;

        std::cout << "\n========== 游戏开始 ==========\n";
        board_.display();

        while (true)
        {
            if (currentPlayer == Player::HUMAN)
            {
                humanMove();
            }
            else
            {
                computerMove();
            }

            board_.display();

            Player winner = GameLogic::checkWinner(board_.getBoard());
            if (winner != Player::NONE)
            {
                announceResult(winner);
                break;
            }

            /// 切换玩家
            currentPlayer = (currentPlayer == Player::HUMAN) ? Player::COMPUTER : Player::HUMAN;
        }
    }

    /// @brief 处理人类玩家落子
    void humanMove()
    {
        std::cout << "\n--- 您的回合 ---\n";
        int move = InputHandler::getNumberInput("请选择落子位置", 0, Constants::BOARD_SIZE - 1);

        while (!board_.placeMove(move, humanSymbol_))
        {
            std::cout << "该位置已被占用或无效，请重新选择！\n";
            move = InputHandler::getNumberInput("请选择落子位置", 0, Constants::BOARD_SIZE - 1);
        }

        std::cout << "您选择了位置 " << move << "\n";
    }

    /// @brief 处理电脑玩家落子
    void computerMove()
    {
        std::cout << "\n--- 电脑思考中 ---\n";
        int move = aiPlayer_->getBestMove(board_.getBoard());

        if (move != -1 && board_.placeMove(move, computerSymbol_))
        {
            std::cout << "电脑选择了位置 " << move << "\n";
        }
        else
        {
            std::cout << "错误：无法找到有效落子位置\n";
        }
    }

    /// @brief 宣布游戏结果
    /// @param winner 获胜者类型
    void announceResult(Player winner)
    {
        std::cout << "\n========== 游戏结束 ==========\n";

        switch (winner)
        {
            case Player::HUMAN:
                std::cout << "🎉 恭喜！您获胜了！\n";
                break;
            case Player::COMPUTER:
                std::cout << "🤖 电脑获胜！再接再厉！\n";
                break;
            case Player::TIE:
                std::cout << "🤝 平局！旗鼓相当！\n";
                break;
            default:
                std::cout << "未知游戏结果\n";
                break;
        }

        std::cout << "===============================\n";
    }

    /// @brief 重置游戏状态（开始新游戏时调用）
    void resetGame()
    {
        board_.clear();

        /// 如果需要交换先手
        if (!humanTurnFirst_)
        {
            humanTurnFirst_ = InputHandler::getYesNoInput("您想要先手吗？");

            delete aiPlayer_;

            if (humanTurnFirst_)
            {
                humanSymbol_    = static_cast<char>(Symbol::X);
                computerSymbol_ = static_cast<char>(Symbol::O);
                aiPlayer_       = new AIPlayer(computerSymbol_);
            }
            else
            {
                humanSymbol_    = static_cast<char>(Symbol::O);
                computerSymbol_ = static_cast<char>(Symbol::X);
                aiPlayer_       = new AIPlayer(computerSymbol_);
            }
        }
    }
};

/// @brief 程序主函数
/// @return 程序退出码
int main()
{
    /// 设置本地化（支持中文输出）
    setlocale(LC_ALL, "zh_CN.UTF-8");

    std::cout << "欢迎来到井字棋游戏！\n";

    try
    {
        TicTacToeGame game;
        game.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "游戏发生错误: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "游戏发生未知错误" << std::endl;
        return 1;
    }

    return 0;
}
