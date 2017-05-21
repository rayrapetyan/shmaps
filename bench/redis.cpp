#include "./redis.h"

#include <iostream>

#include <benchmark/benchmark.h>
#include <hiredis/hiredis.h>

#include "./conf.h"

class RedisFixture : public ::benchmark::Fixture {
public:
    RedisFixture() {
        //std::cout << "RedisFixture ctor" << std::endl;
        c = redisConnect("127.0.0.1", 6379);
        assert(!(c == NULL || c->err));
        redisReply *reply = static_cast<redisReply *>(redisCommand(c, "FLUSHDB"));
        assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    }

    void SetUp() {
        //std::cout << "RedisFixture SetUp" << std::endl;
    }

    void TearDown() {
        //std::cout << "RedisFixture TearDown" << std::endl;

    }

    ~RedisFixture() {
        //std::cout << "RedisFixture dtor" << std::endl;
        redisFree(c);
    }

    redisContext *c;

};

BENCHMARK_F(RedisFixture, BM_RedisSetString)(benchmark::State &st) {
    redisReply *reply;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            reply = static_cast<redisReply *>(redisCommand(c, "SET %d %d EX %d", i, i, el_expires));
            assert(reply->type == REDIS_REPLY_STATUS);
            //assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
            freeReplyObject(reply);
        }
    }
}

BENCHMARK_F(RedisFixture, BM_RedisGetString)(benchmark::State &st) {
    redisReply *reply;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            reply = static_cast<redisReply *>(redisCommand(c, "GET %d", i));
            assert(reply->type == REDIS_REPLY_STRING && (reply->str == std::to_string(i)));
            freeReplyObject(reply);
        }
    }
}