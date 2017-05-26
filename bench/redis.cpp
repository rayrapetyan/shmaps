#include "./redis.h"

#include <iostream>

#include <benchmark/benchmark.h>
#include <hiredis/hiredis.h>

#include "./conf.h"

class RedisFixture : public ::benchmark::Fixture {
public:
    RedisFixture() {
        //std::cout << "RedisFixture ctor" << std::endl;
        c = redisConnectWithTimeout("127.0.0.1", 6379, {5, 0});
        if ((c == NULL || c->err)) {
            std::cout << "error initializing redis connection" << std::endl;
            exit(1);
        }
        redisReply *reply = static_cast<redisReply *>(redisCommand(c, "FLUSHDB"));
        assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    }

    void SetUp(const ::benchmark::State& state) {
        //std::cout << "RedisFixture SetUp" << std::endl;
    }

    void TearDown(const ::benchmark::State& state) {
        //std::cout << "RedisFixture TearDown" << std::endl;

    }

    ~RedisFixture() {
        //std::cout << "RedisFixture dtor" << std::endl;
        redisFree(c);
    }

    redisContext *c;

};

BENCHMARK_F(RedisFixture, BM_RedisSetInt)(benchmark::State &st) {
    redisReply *reply;
    while (st.KeepRunning()) {
        reply = static_cast<redisReply *>(redisCommand(c, "FLUSHDB"));
        assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
        for (int i = 0; i < el_num; ++i) {
            reply = static_cast<redisReply *>(redisCommand(c, "SET %d %d EX %d", i, i, el_expires));
            assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
            freeReplyObject(reply);
        }
    }
}

BENCHMARK_F(RedisFixture, BM_RedisSetGetInt)(benchmark::State &st) {
    redisReply *reply;
    while (st.KeepRunning()) {
        reply = static_cast<redisReply *>(redisCommand(c, "FLUSHDB"));
        assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
        for (int i = 0; i < el_num; ++i) {
            reply = static_cast<redisReply *>(redisCommand(c, "SET %d %d EX %d", i, i, el_expires));
            assert(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
            freeReplyObject(reply);

            reply = static_cast<redisReply *>(redisCommand(c, "GET %d", i));
            assert(reply->type == REDIS_REPLY_STRING && (reply->str == std::to_string(i)));
            freeReplyObject(reply);
        }
    }
}