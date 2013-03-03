require "time"

leftLog = File.new(ARGV[0], "r")
rightLog = File.new(ARGV[1], "r")

readLeft = true
readRight = true

while readLeft or readRight
	leftLine = leftLog.gets if readLeft
	rightLine = rightLog.gets if readRight

	leftTimestamp = Time.iso8601(leftLine.split(" ")[0]).to_f if leftLine
	rightTimestamp = Time.iso8601(rightLine.split(" ")[0]).to_f if rightLine

	if leftLine and leftTimestamp <= rightTimestamp
		puts "L<< #{leftLine}" if leftLine
		readLeft = true
		readRight = false
	elsif rightLine
		puts ">>R #{rightLine}" if rightLine
		readLeft = false
		readRight = true
	else
		readLeft = false
		readRight = false
	end
end
