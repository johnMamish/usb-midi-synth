dsp_test: ../../firmware/dsp_util.c test_dsp_util.c
	gcc -std=gnu99 -I../../firmware $^ -g -o  test_dsp_util -lm

test_audio_gen: ../../firmware/dsp_util.c test_audio_gen.c
	gcc -std=gnu99 -I../../firmware $^ -g -o  test_audio_gen -lm
