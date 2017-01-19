

#define CLIENT_TEST_PORT (8866)
#define SERVER_TEST_PORT (8867)
#define TEST_TEXT "wxy goog."

enum nat_type_t{
	NO_NAT = 0,
	FULL_CONE_NAT = 1,
	ADDR_STRICT_CONE_NAT = 2,
	PORT_ADDR_STRICT_CONE_NAT = 3,
	SYMMETRIC_NAT = 4,
	NAT_TYPE_MAX
};

