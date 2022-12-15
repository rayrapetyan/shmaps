SHMAPS_SEG_SIZE = 2147483648

.PHONY: reset
reset:
	docker build --target reset --build-arg SHMAPS_SEG_SIZE=$(SHMAPS_SEG_SIZE) -t shmaps-reset:latest .
	docker run --rm --name=shmaps-reset \
		-v /dev/shm:/dev/shm shmaps-reset:latest \
		bash -c "/build/src/reset/build/reset"

.PHONY: test
test: reset
	docker build --target test --build-arg SHMAPS_SEG_SIZE=$(SHMAPS_SEG_SIZE) -t shmaps-test:latest .
	docker run --rm --name=shmaps-test \
		-v /dev/shm:/dev/shm shmaps-test:latest \
		bash -c "/build/src/test/build/test"

.PHONY: bench
bench: reset
	docker build --target bench --build-arg SHMAPS_SEG_SIZE=$(SHMAPS_SEG_SIZE) -t shmaps-bench:latest .
	docker run --rm \
		--name=shmaps-bench \
		--net=host \
		-v /dev/shm:/dev/shm shmaps-bench:latest \
		bash -c "\
			redis-server --save "" --appendonly no --daemonize yes \
			&& /build/src/bench/build/bench --benchmark_time_unit=ms \
		"
