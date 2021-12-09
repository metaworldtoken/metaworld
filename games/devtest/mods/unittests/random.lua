function unittests.test_random()
	-- Try out PseudoRandom
	metaworld.log("action", "[unittests] Testing PseudoRandom ...")
	local pseudo = PseudoRandom(13)
	assert(pseudo:next() == 22290)
	assert(pseudo:next() == 13854)
	metaworld.log("action", "[unittests] PseudoRandom test passed!")
	return true
end

