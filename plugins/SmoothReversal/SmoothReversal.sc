SmoothReversal : UGen {
	*ar { |bufnum, playbackRate, switchDirectionTrigger, threshold=0.0001|
		^this.multiNew('audio', bufnum, K2A.ar(BufRateScale.kr(bufnum)) * playbackRate, switchDirectionTrigger, threshold=0.0001)
	}
	checkInputs {
		/* TODO */
		^this.checkValidInputs;
	}
}
