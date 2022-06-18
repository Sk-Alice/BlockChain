#include <iostream>
#include <vector>
#include <sstream>
#include "openssl/sha.h"

#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")

using namespace std;

/* 交易信息*/
class Transaction
{
public:
    // 交易的发送方，接收方
    string from, to;
    // 交易数量
    int amount;

    Transaction() = default;
    Transaction(string from, string to, int amount)
    {
        this->from = from;
        this->to = to;
        this->amount = amount;
    }
};

class Block
{
public:
    vector<Transaction*> transactions;
    string previousHash, hash;
    // 交易时间
    time_t timeStamp;
    // 随机值
    int nonce;
    Block(vector<Transaction*> transactions, string previousHash)
    {
        this->nonce = 1;
        this->transactions = transactions;
        this->previousHash = previousHash;
        this->timeStamp = time(0);
        this->hash = this->computeHash();
    }

    // 输出交易信息
    string getTransactions()
    {
        ostringstream oss;

        for (int i = 0; i < this->transactions.size(); i++)
        {
            oss << "{\"from\":\"" << this->transactions[i]->from
                << "\",\"to\":\"" << this->transactions[i]->to
                << "\",\"amount\":" << this->transactions[i]->amount
                << "},";
        }

        string result = oss.str();

        return result;
    }

    // 计算每个Block的hash值
    string computeHash()
    {
        unsigned char mdStr[33] = { 0 };
        const string& srcStr =
            this->getTransactions() +
            this->previousHash +
            to_string(this->nonce) +
            to_string(this->timeStamp);

        // 调用sha256哈希
        SHA256((const unsigned char*)(srcStr.c_str()), srcStr.size(), mdStr);

        string encodedStr = static_cast<string>((const char*)mdStr);
        char buf[65] = { 0 };
        char tmp[3] = { 0 };

        for (int i = 0; i < 32; i++)
        {
            // X 表示以十六进制形式输出
            // 02表示不足两位，前面补0输出；如果超过两位，则实际输出
            // 将mdStr[i]保存到tmp中
            sprintf_s(tmp, "%02x", mdStr[i]);
            // 追加tmp到buf中
            strcat_s(buf, tmp);
        }
        // 后面都是0，从32字节截断
        buf[32] = '\0';
        string encodedHexStr = static_cast<string>(buf);
        return encodedHexStr;
    }

    // 设置挖矿难度标准--开头前n位为0的hash
    string getAnswer(int difficulty)
    {
        // 难度：开头前n位为0的hash
        string answer = "";

        for (int i = 0; i < difficulty; ++i)
        {
            answer += "0";
        }
        return answer;
    }

    // 计算符合区块链难度要求的hash值 -- 挖矿
    void mine(int difficulty)
    {
        while (true)
        {
            this->hash = this->computeHash();
            if (this->hash.substr(0, difficulty) != this->getAnswer(difficulty))
            {
                this->nonce++;
                this->hash = this->computeHash();
            }
            else
            {
                break;
            }
        }
        cout << "挖矿结束\t" << this->hash << "\t" << this->nonce << endl;
    }
};

class Chain
{
public:
    vector<Block*> chain;
    vector<Transaction*> transactionPool;
    // 挖矿奖励
    int minerReward;
    // 挖矿难度
    int difficulty;
    Chain()
    {
        this->minerReward = 50;
        this->difficulty = 1;
        this->chain.push_back(this->bigBang());
    }

    // 返回祖先区块
    Block* bigBang()
    {
        // 第一个祖先区块，previousHash为空
        Block* genesisBlock = new Block(this->transactionPool, "");
        return genesisBlock;
    }

    // 找到最近的一个Block---即previousBlock
    Block* getLatestBlock()
    {
        return this->chain[this->chain.size() - 1];
    }

    // 添加transaction 到 transactionPool
    void addTransaction(Transaction* transaction)
    {
        this->transactionPool.push_back(transaction);
    }

    // 从外部添加block到chain中
    void addBlockToChain(Block* newBlock)
    {
        // 找到最近一个Block的hash,该hash就是新区块的previousHash
        newBlock->previousHash = this->getLatestBlock()->hash;
        newBlock->hash = newBlock->computeHash();
        // 挖矿--获取符满足一个区块链设置的条件的hash值
        newBlock->mine(this->difficulty);
        this->chain.push_back(newBlock);
    }

    // 矿工的交易池
    void mineTransactionPool(string minerRewardAddress)
    {
        // 矿工奖励
        Transaction* minerRewardTransaction = new Transaction(
            "",
            minerRewardAddress,
            this->minerReward
        );
        this->transactionPool.push_back(minerRewardTransaction);
        // 挖矿
        Block* newBlock = new Block(
            this->transactionPool,
            this->getLatestBlock()->hash
        );
        newBlock->mine(difficulty);
        // 添加区块到区块链
        this->chain.push_back(newBlock);

        // 清空 transactionPool
        this->transactionPool.clear();
    }

    // 验证当前区块链的数据是否合法
    bool validateChain()
    {
        if (this->chain.size() == 1)
        {
            if (this->chain[0]->hash != this->chain[0]->computeHash())
            {
                return false;
            }
            return true;
        }

        // 从第二个区块开始验证
        for (int i = 1; i <= this->chain.size() - 1; ++i)
        {
            Block* blockToValidate = this->chain[i];
            // 验证当前的数据有没有被篡改
            if (blockToValidate->hash != blockToValidate->computeHash())
            {
                cout << blockToValidate->hash << "的数据被篡改" << endl;
                return false;
            }
            // 验证区块的previousHash是否等于previous区块的hash
            Block* previousBlock = this->chain[i - 1];
            if (blockToValidate->previousHash != previousBlock->hash)
            {
                cout << blockToValidate->hash << "与前区块链接断裂" << endl;
                return false;
            }
        }
        return true;
    }
};

ostream& operator<<(ostream& os, Block* myBlock)
{
    os << "Block {\n"
        << "\ttransactions : " << myBlock->getTransactions()
        << "\n\tpreviousHash : " << myBlock->previousHash
        << "\n\thash : " << myBlock->hash
        << "\n}"
        << endl;
    return os;
}

int main()
{
    Chain* myChain = new Chain();

    Transaction* t1 = new Transaction("add1", "add2", 10);
    Transaction* t2 = new Transaction("add2", "add1", 5);

    myChain->addTransaction(t1);
    myChain->addTransaction(t2);

    myChain->mineTransactionPool("add3");


    /* 该段注释若直接使用会报错，因为Block构造函数的参数后期有所改变*/
    //Block* block1 = new Block("转账十元", "");
    //myChain->addBlockToChain(block1);
    //Block* block2 = new Block("转账十个十元", "");
    //myChain->addBlockToChain(block2);
    //Block* block3 = new Block("转账二十个十元", "");
    //myChain->addBlockToChain(block3);
    //Block* block4 = new Block("转账三十个十元", "");
    //myChain->addBlockToChain(block4);

    //// 尝试篡改数据
    //myChain->chain[1]->data = "转账一百个十元";
    //myChain->chain[1]->mine(myChain->difficulty);

    for (int i = 0; i < myChain->chain.size(); i++)
    {
        cout << myChain->chain[i];
    }

    return 0;
}