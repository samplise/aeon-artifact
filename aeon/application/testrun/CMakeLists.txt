SET(APPS contextrun)
SET(SERVICES TcpTransport 
	#BTreeServer BTreeClient
	#BinaryTreeApp
	#HaloPresenceService
	#TPCWMinApp
	#PageRankApp
	#HaloPresenceServiceClient HaloPresenceServiceServer
	#TreeTPCCAppServer TreeTPCCAppClient
	#TagItemShareAppServer TagItemShareAppClient
	#DistributedDAGTestAppServer DistributedDAGTestAppClient
	#WTFAppClient WTFAppServer
	#TagAppServer TagAppClient
	#Chirper
	#HelloWorldApp
	GameApp
)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
