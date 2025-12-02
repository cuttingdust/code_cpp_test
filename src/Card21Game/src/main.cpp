#include <Windows.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#include <memory>
#include <format>

/// ============================
/// 扑克牌类
/// ============================
class Card
{
public:
    /// 牌面值枚举
    enum class Rank
    {
        ACE = 1, ///< A可计为1或11
        TWO,     ///< 2
        THREE,   ///< 3
        FOUR,    ///< 4
        FIVE,    ///< 5
        SIX,     ///< 6
        SEVEN,   ///< 7
        EIGHT,   ///< 8
        NINE,    ///< 9
        TEN,     ///< 10
        JACK,    ///< J
        QUEEN,   ///< Q
        KING     ///< K
    };

    /// 花色枚举
    enum class Suit
    {
        CLUBS,    ///< ♣
        DIAMONDS, ///< ♦
        HEARTS,   ///< ♥
        SPADES    ///< ♠
    };

    Card(Rank rank = Rank::ACE, Suit suit = Suit::SPADES, bool isFaceUp = true);
    using Ptr = std::shared_ptr<Card>;

    template <typename... Args>
    static auto create(Args&&... args) -> Card::Ptr
    {
        return std::make_shared<Card>(std::forward<Args>(args)...);
    }


public:
    /// 获取牌面值（Ace可计为1或11）
    int getValue() const;

    /// 翻转牌面
    void flip();

    /// 重载输出运算符
    friend std::ostream& operator<<(std::ostream& os, const Card& card);

    /// 检查是否面朝上
    bool isFaceUp() const
    {
        return isFaceUp_;
    }

    /// 获取牌面
    Rank getRank() const
    {
        return rank_;
    }

    /// 获取花色
    Suit getSuit() const
    {
        return suit_;
    }

private:
    Rank rank_;     ///< 牌面值
    Suit suit_;     ///< 花色
    bool isFaceUp_; ///< 是否面朝上
};

Card::Card(Rank rank, Suit suit, bool isFaceUp) : rank_(rank), suit_(suit), isFaceUp_(isFaceUp)
{
}

int Card::getValue() const
{
    if (!isFaceUp_)
    {
        return 0; ///< 牌面向下时值为0
    }

    int value = static_cast<int>(rank_);
    return (value > 10) ? 10 : value; ///< J、Q、K都计为10点
}

void Card::flip()
{
    isFaceUp_ = !isFaceUp_;
}

/// ============================
/// 手牌类（基础类）
/// ============================
class Hand
{
public:
    Hand();
    virtual ~Hand() = default;

    using Ptr = std::shared_ptr<Hand>;
    static auto create() -> Hand::Ptr
    {
        return std::make_shared<Hand>();
    }

public:
    /// 添加一张牌
    void addCard(const Card::Ptr& card);

    /// 清空手牌
    void clear();

    /// 计算手牌总点数
    int getTotal() const;

    /// 检查手牌是否为空
    bool isEmpty() const
    {
        return cards_.empty();
    }

    /// 获取手牌数量
    size_t size() const
    {
        return cards_.size();
    }

    /// 检查是否包含Ace
    bool containsAce() const;

protected:
    std::vector<Card::Ptr> cards_;
};

Hand::Hand()
{
    cards_.reserve(12); ///< 预留足够空间
}

void Hand::addCard(const Card::Ptr& card)
{
    cards_.push_back(card);
}

void Hand::clear()
{
    cards_.clear();
}

bool Hand::containsAce() const
{
    return std::ranges::any_of(cards_,
                               [](const Card::Ptr& card) -> bool
                               {
                                   return card->getValue() == 1 && card->isFaceUp(); ///< 只考虑面朝上的Ace
                               });
}

int Hand::getTotal() const
{
    if (cards_.empty())
    {
        return 0;
    }

    if (!cards_[0]->isFaceUp())
    {
        return 0; ///< 第一张牌面向下，不计算点数
    }

    /// 计算基本点数（Ace计为1）
    int total = 0;
    for (const auto& card : cards_)
    {
        total += card->getValue();
    }

    /// 如果包含Ace且点数不超过11，将Ace计为11点
    if (containsAce() && total <= 11)
    {
        total += 10; ///< 将Ace的1点改为11点，因此额外加10
    }

    return total;
}

/// ============================
/// 通用玩家类
/// ============================
class GenericPlayer : public Hand
{
public:
    GenericPlayer(std::string name = "");
    ~GenericPlayer() override = default;

public:
    /// 纯虚函数：是否要牌
    virtual bool isHitting() const = 0;

    /// 检查是否爆牌（超过21点）
    bool isBusted() const
    {
        return getTotal() > 21;
    }

    /// 宣布爆牌
    void bust() const;

    /// 获取玩家名字
    std::string getName() const
    {
        return name_;
    }

    /// 重载输出运算符
    friend std::ostream& operator<<(std::ostream& os, const GenericPlayer& player);

protected:
    std::string name_; ///< 玩家姓名
};

GenericPlayer::GenericPlayer(std::string name) : name_(std::move(name))
{
}

void GenericPlayer::bust() const
{
    std::cout << std::format("{} 爆牌了！\n", name_);
}

/// ============================
/// 真实玩家类
/// ============================
class Player : public GenericPlayer
{
public:
    Player(std::string name = "");

    /// 实现是否要牌
    virtual bool isHitting() const override;

    /// 获胜
    void win() const;

    /// 失败
    void lose() const;

    /// 平局
    void push() const;
};

Player::Player(std::string name) : GenericPlayer(std::move(name))
{
}

bool Player::isHitting() const
{
    char response;
    while (true)
    {
        std::cout << format("{}, 是否要牌？(Y/N): ", name_);
        std::cin >> response;
        response = toupper(response);

        if (response == 'Y' || response == 'N')
        {
            return response == 'Y';
        }
        std::cout << "请输入 Y 或 N！\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void Player::win() const
{
    std::cout << format("{} 获胜！\n", name_);
}

void Player::lose() const
{
    std::cout << format("{} 输了！\n", name_);
}

void Player::push() const
{
    std::cout << format("{} 平局！\n", name_);
}

/// ============================
/// 庄家类
/// ============================
class House : public GenericPlayer
{
public:
    House(std::string name = "庄家");

    /// 庄家要牌策略：点数不超过16时必须加牌
    virtual bool isHitting() const override;

    /// 翻转第一张牌
    void flipFirstCard() const;
};

House::House(std::string name) : GenericPlayer(std::move(name))
{
}

bool House::isHitting() const
{
    return getTotal() <= 16; ///< 庄家策略：16点及以下必须加牌
}

void House::flipFirstCard() const
{
    if (!cards_.empty())
    {
        cards_[0]->flip();
    }
}

/// ============================
/// 牌堆类
/// ============================
class Deck : public Hand
{
public:
    Deck();

    /// 生成标准52张牌
    void populate();

    /// 洗牌
    void shuffle();

    /// 发一张牌给玩家
    void deal(Hand& hand);

    /// 给玩家发额外牌（根据玩家决定是否要牌）
    void additionalCards(GenericPlayer& player);

private:
    std::mt19937 rng_; ///< 现代C++随机数生成器
};

Deck::Deck() : rng_(std::random_device{}())
{
    populate(); /// 填充
    shuffle();  /// 打乱
}

void Deck::populate()
{
    clear();

    /// 生成所有52张牌的组合
    for (int s = static_cast<int>(Card::Suit::CLUBS); s <= static_cast<int>(Card::Suit::SPADES); ++s)
    {
        for (int r = static_cast<int>(Card::Rank::ACE); r <= static_cast<int>(Card::Rank::KING); ++r)
        {
            auto card = Card::create(static_cast<Card::Rank>(r), static_cast<Card::Suit>(s));
            addCard(card);
        }
    }
}

void Deck::shuffle()
{
    /// 使用C++17的shuffle算法
    std::ranges::shuffle(cards_, rng_);
}

void Deck::deal(Hand& hand)
{
    if (!cards_.empty())
    {
        hand.addCard(cards_.back());
        cards_.pop_back();
    }
}

void Deck::additionalCards(GenericPlayer& player)
{
    std::cout << std::endl;

    /// 循环直到玩家不要牌或爆牌
    while (!player.isBusted() && player.isHitting())
    {
        deal(player);
        std::cout << player << std::endl;

        if (player.isBusted())
        {
            player.bust();
        }
    }
}

/// ============================
/// 游戏主逻辑类
/// ============================
class Game
{
public:
    Game(const std::vector<std::string>& playerNames);

    /// 开始一轮游戏
    void play();

private:
    /// 初始化游戏
    void initialize();

    /// 结算游戏结果
    void settleResults();

private:
    Deck                deck_;    ///< 牌堆
    House               house_;   ///< 庄家
    std::vector<Player> players_; ///< 玩家列表
};

Game::Game(const std::vector<std::string>& playerNames)
{
    /// 创建所有玩家
    for (const auto& name : playerNames)
    {
        players_.emplace_back(name);
    }

    initialize();
}

void Game::initialize()
{
    /// 重新生成牌堆并洗牌
    deck_.populate();
    deck_.shuffle();
}

void Game::play()
{
    std::cout << "\n========== 开始新游戏 ==========\n";

    /// 第一轮发牌：每人两张
    for (int i = 0; i < 2; ++i)
    {
        for (auto& player : players_)
        {
            deck_.deal(player);
        }
        deck_.deal(house_);
    }

    /// 庄家隐藏第一张牌
    house_.flipFirstCard();

    /// 显示初始牌面
    std::cout << "\n初始牌面：\n";
    for (const auto& player : players_)
    {
        std::cout << player << std::endl;
    }
    std::cout << house_ << std::endl;

    /// 玩家要牌阶段
    std::cout << "\n=== 玩家要牌阶段 ===\n";
    for (auto& player : players_)
    {
        deck_.additionalCards(player);
    }

    /// 庄家翻牌并要牌
    std::cout << "\n=== 庄家行动 ===\n";
    house_.flipFirstCard();
    std::cout << house_ << std::endl;
    deck_.additionalCards(house_);

    /// 结算结果
    settleResults();

    /// 清理手牌，准备下一轮
    for (auto& player : players_)
    {
        player.clear();
    }
    house_.clear();

    /// 检查是否需要重新洗牌
    if (deck_.size() < 20)
    {
        std::cout << "\n牌堆剩余不足，重新洗牌...\n";
        initialize();
    }
}

void Game::settleResults()
{
    std::cout << "\n========== 游戏结果 ==========\n";

    /// 庄家爆牌：所有未爆牌的玩家获胜
    if (house_.isBusted())
    {
        std::cout << "庄家爆牌！\n";
        for (auto& player : players_)
        {
            if (!player.isBusted())
            {
                player.win();
            }
        }
    }
    else
    {
        /// 庄家未爆牌：比较点数
        int houseTotal = house_.getTotal();
        std::cout << std::format("庄家点数: {}\n", houseTotal);

        for (auto& player : players_)
        {
            if (!player.isBusted())
            {
                int playerTotal = player.getTotal();
                std::cout << format("{} 点数: {}\n", player.getName(), playerTotal);

                if (playerTotal > houseTotal)
                {
                    player.win();
                }
                else if (playerTotal < houseTotal)
                {
                    player.lose();
                }
                else
                {
                    player.push();
                }
            }
        }
    }
}

/// ============================
/// 辅助函数：输出运算符重载
/// ============================
std::ostream& operator<<(std::ostream& os, const Card& card)
{
    static const std::vector<std::string> ranks = { "0", "A", "2", "3",  "4", "5", "6",
                                                    "7", "8", "9", "10", "J", "Q", "K" };

    static const std::vector<std::string> suits = {
        "♣", "♦", "♥", "♠" ///< Unicode花色符号
    };

    if (card.isFaceUp())
    {
        int rankIdx = static_cast<int>(card.getRank());
        int suitIdx = static_cast<int>(card.getSuit());
        os << format("{}{}", ranks[rankIdx], suits[suitIdx]);
    }
    else
    {
        os << "??"; ///< 牌面向下显示问号
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const GenericPlayer& player)
{
    os << std::format("{:10}: ", player.getName());

    if (player.isEmpty())
    {
        os << "<空>";
    }
    else
    {
        /// 显示所有手牌
        for (const auto& card : player.cards_)
        {
            os << *card << " ";
        }

        /// 显示总点数
        int total = player.getTotal();
        if (total > 0)
        {
            os << std::format("({}点)", total);
        }
    }

    return os;
}

/// ============================
/// 主函数
/// ============================
int main()
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    // SetConsoleOutputCP(CP_UTF8);
    // SetConsoleCP(CP_UTF8);

    std::cout << "========== 欢迎来到21点游戏 ==========\n";
    std::cout << "规则说明：\n";
    std::cout << "1. 目标：使手牌点数最接近21点但不爆牌\n";
    std::cout << "2. A可计为1点或11点\n";
    std::cout << "3. J、Q、K计为10点\n";
    std::cout << "4. 庄家必须加到16点为止\n";
    std::cout << "====================================\n\n";

    /// 获取玩家数量
    int numPlayers = 0;
    while (numPlayers < 1 || numPlayers > 7)
    {
        std::cout << "请输入玩家数量 (1-7): ";
        std::cin >> numPlayers;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            numPlayers = 0;
        }
    }

    /// 获取玩家姓名
    std::vector<std::string> playerNames;
    for (int i = 0; i < numPlayers; ++i)
    {
        std::string name;
        std::cout << std::format("请输入玩家{}的名字: ", i + 1);
        std::cin >> name;
        playerNames.push_back(name);
    }

    /// 创建游戏实例
    Game game(playerNames);

    /// 游戏主循环
    char playAgain = 'Y';
    while (toupper(playAgain) == 'Y')
    {
        game.play();

        std::cout << "\n是否继续游戏？(Y/N): ";
        std::cin >> playAgain;
    }

    std::cout << "\n感谢游玩21点游戏！再见！\n";
    std::cout << "按Enter键退出...";
    std::cin.ignore();
    std::cin.get();

    return 0;
}
