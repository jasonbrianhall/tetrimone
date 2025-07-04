// American-style propaganda messages for patriotic mode
const std::vector<std::string> AMERICAN_PROPAGANDA_MESSAGES = {
    // Core American values
    "FREEDOM BLOCKS FALL WHERE THEY CHOOSE!",
    "LIBERTY AND TETRIMINOS FOR ALL!",
    "DEMOCRATIC BLOCK PLACEMENT BY THE PEOPLE!",
    "YOUR RIGHT TO ROTATE BLOCKS SHALL NOT BE INFRINGED!",
    "LIFE, LIBERTY, AND THE PURSUIT OF LINE CLEARS!",
    "THESE BLOCKS ARE SELF-EVIDENT TRUTHS!",
    "MANIFEST DESTINY OF FALLING BLOCKS!",
    "GIVE ME LIBERTY OR GIVE ME GAME OVER!",
    "FOUR SCORE AND SEVEN LINES AGO...",
    "WE HOLD THESE BLOCKS TO BE SELF-EVIDENT!",
    
    // Capitalism and free market
    "SUPPLY AND DEMAND FOR PERFECT BLOCKS!",
    "FREE MARKET TETRIMINO COMPETITION!",
    "BLOCKS TRICKLE DOWN FROM TOP TO BOTTOM!",
    "INVISIBLE HAND OF THE MARKET GUIDES YOUR PIECES!",
    "ENTREPRENEURIAL SPIRIT IN EVERY BLOCK DROP!",
    "PRIVATIZED BLOCK PLACEMENT SYSTEM!",
    "COMPETITIVE ADVANTAGE THROUGH STRATEGIC STACKING!",
    "CORPORATE SYNERGY IN BLOCK COMBINATIONS!",
    "MAXIMIZE SHAREHOLDER VALUE WITH EVERY LINE!",
    "QUARTERLY EARNINGS EXCEED EXPECTATIONS!",
    "BOOTSTRAP YOUR WAY TO BLOCK SUCCESS!",
    "RUGGED INDIVIDUALISM IN TETRIMINO MANAGEMENT!",
    "THE AMERICAN DREAM: OWNING YOUR OWN BLOCKS!",
    
    // Military and patriotism
    "SUPPORT OUR TROOPS AND SUPPORT YOUR BLOCKS!",
    "THESE COLORS DON'T RUN... THEY STACK!",
    "SEMPER FI-DELITY TO BLOCK PLACEMENT!",
    "HOOAH! OUTSTANDING TETRIMINO PERFORMANCE!",
    "MARINES: THE FEW, THE PROUD, THE BLOCK STACKERS!",
    "ARMY STRONG BLOCK FOUNDATION!",
    "NAVY SEAL TEAM SIX BLOCK OPERATION!",
    "AIR FORCE THUNDERBIRDS PRECISION DROPPING!",
    "COAST GUARD RESCUES DROWNING BLOCKS!",
    "SPACE FORCE PROTECTS ORBITAL TETRIMINOS!",
    "THANK YOU FOR YOUR SERVICE TO BLOCK FREEDOM!",
    "PURPLE HEART FOR WOUNDED BLOCK VETERANS!",
    "MEDAL OF HONOR TETRIMINO RECIPIENT!",
    
    // Constitution and founding fathers
    "FIRST AMENDMENT PROTECTS YOUR RIGHT TO ROTATE!",
    "SECOND AMENDMENT: RIGHT TO BEAR BLOCKS!",
    "FOURTH AMENDMENT: NO UNREASONABLE BLOCK SEARCHES!",
    "GEORGE WASHINGTON CROSSED THE DELAWARE FOR THIS FREEDOM!",
    "BENJAMIN FRANKLIN: 'A BLOCK SAVED IS A BLOCK EARNED!'",
    "THOMAS JEFFERSON DECLARES BLOCK INDEPENDENCE!",
    "ALEXANDER HAMILTON'S FINANCIAL PLAN FOR BLOCKS!",
    "JOHN ADAMS: 'BLOCKS AND LIBERTY FOREVER!'",
    "JAMES MADISON: FATHER OF THE BLOCK CONSTITUTION!",
    "ABRAHAM LINCOLN: 'FOUR SCORE AND SEVEN BLOCKS AGO!'",
    "TEDDY ROOSEVELT: 'SPEAK SOFTLY AND CARRY A BIG BLOCK!'",
    "JFK: 'ASK NOT WHAT YOUR BLOCKS CAN DO FOR YOU!'",
    
    // American exceptionalism
    "AMERICAN BLOCKS ARE THE BEST BLOCKS!",
    "MADE IN USA TETRIMINO QUALITY!",
    "WORLD'S GREATEST BLOCK STACKING NATION!",
    "AMERICAN INGENUITY IN BLOCK TECHNOLOGY!",
    "LAND OF THE FREE, HOME OF THE BLOCKS!",
    "SHINING CITY UPON A HILL... OF BLOCKS!",
    "BEACON OF HOPE FOR OPPRESSED BLOCKS WORLDWIDE!",
    "LEADER OF THE FREE BLOCK WORLD!",
    "AMERICAN BLOCKS SPREAD DEMOCRACY!",
    "GREATEST BLOCK NATION IN HISTORY!",
    "ARSENAL OF BLOCK DEMOCRACY!",
    
    // Fast food and consumer culture
    "HAVE IT YOUR WAY WITH BLOCK COMBOS!",
    "I'M LOVIN' IT - MCTETRIMINO!",
    "FINGER LICKIN' GOOD BLOCK STACKING!",
    "SUBWAY: EAT FRESH BLOCKS!",
    "PIZZA DELIVERED IN 30 MINUTES OR LESS!",
    "SUPER SIZE YOUR BLOCK COMBINATIONS!",
    "WOULD YOU LIKE FRIES WITH THOSE BLOCKS?",
    "TACO TUESDAY BLOCK SPECIAL!",
    "ALL-YOU-CAN-STACK BLOCK BUFFET!",
    "DRIVE-THRU TETRIMINO SERVICE!",
    "24/7 CONVENIENCE STORE BLOCKS!",
    "BLACK FRIDAY DOORBUSTER BLOCK DEALS!",
    "CYBER MONDAY ONLINE BLOCK SHOPPING!",
    
    // Hollywood and entertainment
    "LIGHTS, CAMERA, BLOCK ACTION!",
    "OSCAR-WINNING TETRIMINO PERFORMANCE!",
    "HOLLYWOOD BLOCKBUSTER BLOCK STACKING!",
    "NETFLIX AND CHILL WITH BLOCKS!",
    "DISNEY MAGIC IN EVERY BLOCK DROP!",
    "MARVEL SUPERHERO BLOCK POWERS!",
    "STAR WARS: THE BLOCKS AWAKEN!",
    "INDIANA JONES AND THE TEMPLE OF BLOCKS!",
    "TRANSFORMERS: BLOCKS IN DISGUISE!",
    "JURASSIC PARK: LIFE FINDS A WAY... TO STACK!",
    "BACK TO THE FUTURE BLOCK TIME TRAVEL!",
    "TOP GUN: MAVERICK BLOCK PILOT!",
    
    // Technology and innovation
    "SILICON VALLEY BLOCK INNOVATION!",
    "APPLE IBLOCK REVOLUTIONARY DESIGN!",
    "GOOGLE SEARCH FOR PERFECT BLOCK PLACEMENT!",
    "FACEBOOK SOCIAL BLOCK NETWORKING!",
    "AMAZON PRIME NEXT-DAY BLOCK DELIVERY!",
    "TESLA ELECTRIC BLOCK VEHICLES!",
    "SPACEX LAUNCHES BLOCKS TO MARS!",
    "MICROSOFT WINDOWS BLOCK OPERATING SYSTEM!",
    "IBM BLOCK COMPUTING SOLUTIONS!",
    "INTEL INSIDE EVERY BLOCK PROCESSOR!",
    "CRYPTOCURRENCY: BLOCKCHAIN TECHNOLOGY!",
    "AI-POWERED BLOCK MACHINE LEARNING!",
    
    // Sports culture
    "SUPER BOWL CHAMPION BLOCK TEAM!",
    "WORLD SERIES OF BLOCK STACKING!",
    "NBA FINALS MVP BLOCK PLAYER!",
    "NFL DRAFT FIRST ROUND BLOCK PICK!",
    "MARCH MADNESS BLOCK TOURNAMENT!",
    "HOMERUN DERBY BLOCK COMPETITION!",
    "OLYMPIC GOLD MEDAL BLOCK PERFORMANCE!",
    "FANTASY FOOTBALL BLOCK LEAGUE!",
    "TAILGATING WITH TETRIMINO BLOCKS!",
    "MONDAY NIGHT FOOTBALL BLOCKS!",
    "COLLEGE GAMEDAY BLOCK PREDICTIONS!",
    "ESPN SPORTSCENTER TOP 10 BLOCKS!",
    
    // State stereotypes
    "TEXAS-SIZED BLOCK COMBINATIONS!",
    "CALIFORNIA GOLD RUSH FOR BLOCKS!",
    "NEW YORK MINUTE BLOCK PLACEMENT!",
    "FLORIDA MAN STACKS BLOCKS UNUSUALLY!",
    "MIDWEST VALUES IN BLOCK STACKING!",
    "SOUTHERN HOSPITALITY FOR ALL BLOCKS!",
    "ALASKA: LAST FRONTIER OF BLOCK STACKING!",
    "HAWAII: ALOHA BLOCK SPIRIT!",
    "VEGAS: WHAT HAPPENS WITH BLOCKS, STAYS WITH BLOCKS!",
    "WISCONSIN CHEESE-POWERED BLOCK ENERGY!",
    "KENTUCKY DERBY FAST BLOCK RACING!",
    "COLORADO HIGH ALTITUDE BLOCK TRAINING!",
    "ALMOST HEAVEN, WEST BLOCKGINIA!",
    
    // Presidential references
    "MAKE BLOCKS GREAT AGAIN!",
    "YES WE CAN STACK BLOCKS!",
    "READ MY LIPS: NO NEW BLOCK TAXES!",
    "I AM NOT A CROOK... AT BLOCK STACKING!",
    "THE BUCK STOPS HERE... WITH BLOCKS!",
    "SPEAK SOFTLY AND CARRY BIG BLOCKS!",
    "ASK NOT WHAT YOUR BLOCKS CAN DO FOR YOU!",
    "MR. GORBACHEV, TEAR DOWN THIS BLOCK WALL!",
    "PRESIDENTIAL BLOCK PARDONS FOR ALL!",
    "EXECUTIVE ORDER: MANDATORY BLOCK STACKING!",
    "CABINET MEETING ABOUT NATIONAL BLOCK POLICY!",
    "STATE OF THE UNION: BLOCKS ARE STRONG!",
    
    // Suburban/middle class life
    "WHITE PICKET FENCE BLOCK PROPERTY!",
    "SOCCER MOM DRIVES BLOCKS TO PRACTICE!",
    "BBQ WEEKEND BLOCK COOKOUT!",
    "HOMEOWNERS ASSOCIATION APPROVES BLOCK DESIGN!",
    "NEIGHBORHOOD WATCH PROTECTS YOUR BLOCKS!",
    "PTA MEETING DISCUSSES BLOCK EDUCATION!",
    "LITTLE LEAGUE BLOCK TEAM PARENTS!",
    "SUBURBAN BLOCK LAWN CARE SERVICES!",
    "CUL-DE-SAC COMMUNITY BLOCK GATHERING!",
    "MINIVAN FAMILY BLOCK ROAD TRIP!",
    "COSTCO BULK BLOCK PURCHASES!",
    "HOME DEPOT WEEKEND BLOCK PROJECTS!",
    
    // Cold War victory themes
    "RONALD REAGAN WOULD BE PROUD OF YOUR BLOCKS!",
    "FREEDOM DEFEATED COMMUNISM, BLOCKS DEFEAT GRAVITY!",
    "BERLIN WALL FELL, YOUR BLOCK WALL RISES!",
    "DEMOCRACY TRIUMPHS OVER BLOCK TYRANNY!",
    "IRON CURTAIN COULDN'T STOP AMERICAN BLOCKS!",
    "CONTAINMENT POLICY FOR ENEMY BLOCK PIECES!",
    "STRATEGIC DEFENSE INITIATIVE FOR BLOCKS!",
    "PEACE THROUGH STRENGTH... AND BLOCKS!",
    "MORNING IN AMERICA FOR BLOCK FREEDOM!",
    "SHINING CITY ON A HILL OF BLOCKS!",
    
    // Space program
    "ONE SMALL STEP FOR BLOCKS, ONE GIANT LEAP FOR BLOCKIND!",
    "HOUSTON, WE HAVE NO PROBLEM WITH THESE BLOCKS!",
    "APOLLO PROGRAM BLOCK MISSION SUCCESS!",
    "SPACE SHUTTLE DISCOVERY CARRIES BLOCKS TO ORBIT!",
    "INTERNATIONAL SPACE STATION BLOCK EXPERIMENTS!",
    "MARS ROVER DISCOVERS ALIEN BLOCKS!",
    "NASA APPROVES YOUR BLOCK TRAJECTORY!",
    "MISSION CONTROL: ALL BLOCKS GO FOR LAUNCH!",
    "ASTRONAUT BLOCK TRAINING PROGRAM!",
    "TANG-POWERED BLOCK ENERGY DRINKS!",
    
    // Economic boom references
    "DOW JONES UP 500 POINTS ON BLOCK NEWS!",
    "NASDAQ HITS RECORD HIGH ON BLOCK TRADING!",
    "WALL STREET LOVES YOUR BLOCK PORTFOLIO!",
    "FEDERAL RESERVE CUTS RATES FOR BLOCK ECONOMY!",
    "GDP GROWTH DRIVEN BY BLOCK INDUSTRY!",
    "UNEMPLOYMENT DOWN, BLOCK EMPLOYMENT UP!",
    "CONSUMER CONFIDENCE HIGH ON BLOCK OUTLOOK!",
    "INFLATION CONTROLLED BY BLOCK MONETARY POLICY!",
    "TRADE SURPLUS IN BLOCK EXPORTS!",
    "ECONOMIC INDICATORS ALL POINT TO BLOCK SUCCESS!",
    
    // Pop culture references
    "FRIENDS: THE ONE WITH ALL THE BLOCKS!",
    "SEINFELD: A SHOW ABOUT NOTHING... BUT BLOCKS!",
    "SURVIVOR: OUTWIT, OUTPLAY, OUTSTACK!",
    "AMERICAN IDOL: YOU'RE GOING TO BLOCKYWOOD!",
    "THE BACHELOR: WILL YOU ACCEPT THIS BLOCK?",
    "JEOPARDY: WHAT IS... PERFECT BLOCK STACKING?",
    "WHEEL OF FORTUNE: I'D LIKE TO BUY A BLOCK!",
    "PRICE IS RIGHT: COME ON DOWN FOR BLOCKS!",
    "OPRAH: YOU GET A BLOCK! EVERYBODY GETS BLOCKS!",
    "ELLEN: BE KIND TO YOUR BLOCKS!",
    
    // Internet culture
    "VIRAL BLOCK VIDEO BREAKS THE INTERNET!",
    "TRENDING HASHTAG: #BLOCKSTACKINGCHALLENGE!",
    "YOUTUBE MONETIZATION FOR BLOCK CONTENT!",
    "TIKTOK DANCE FEATURING BLOCK MOVEMENTS!",
    "INSTAGRAM INFLUENCER PROMOTES BLOCK LIFESTYLE!",
    "TWITTER THREAD ABOUT BLOCK STRATEGY!",
    "REDDIT UPVOTES YOUR BLOCK TECHNIQUE!",
    "MEME STATUS: ACHIEVED WITH BLOCKS!",
    "GOING VIRAL WITH BLOCK CONTENT!",
    "INTERNET FAMOUS FOR BLOCK SKILLS!",
    
    // School system
    "STANDARDIZED BLOCK TESTING SCORES EXCELLENT!",
    "COLLEGE SCHOLARSHIP FOR BLOCK ATHLETICS!",
    "STUDENT LOAN FORGIVENESS FOR BLOCK STUDIES!",
    "PTA FUNDRAISER: BLOCK STACKING CARNIVAL!",
    "HONOR ROLL STUDENT IN BLOCK MATHEMATICS!",
    "VALEDICTORIAN SPEECH ABOUT BLOCK SUCCESS!",
    "HOMECOMING KING AND QUEEN OF BLOCKS!",
    "PROM NIGHT BLOCK DANCING COMPETITION!",
    "YEARBOOK SUPERLATIVE: MOST LIKELY TO STACK BLOCKS!",
    "GRADUATION DAY: BLOCKS AND MORTAR BOARDS!",
    
    // Medical/health culture
    "SURGEON GENERAL WARNING: BLOCKS MAY BE ADDICTIVE!",
    "FDA APPROVES BLOCKS FOR THERAPEUTIC USE!",
    "HEALTH INSURANCE COVERS BLOCK THERAPY!",
    "DOCTOR RECOMMENDS DAILY BLOCK VITAMINS!",
    "MENTAL HEALTH BENEFITS OF BLOCK STACKING!",
    "EXERCISE PROGRAM: BLOCK AEROBICS!",
    "NUTRITION FACTS: BLOCKS CONTAIN ZERO CALORIES!",
    "WELLNESS PROGRAM INCLUDES BLOCK MEDITATION!",
    "STRESS RELIEF THROUGH BLOCK ARRANGEMENT!",
    "PHYSICAL THERAPY WITH THERAPEUTIC BLOCKS!",
    
    // Religious references
    "GOD BLESS AMERICA AND THESE BLOCKS!",
    "IN BLOCKS WE TRUST!",
    "ONE NATION UNDER BLOCKS!",
    "BLESSED ARE THE BLOCK STACKERS!",
    "SUNDAY SERVICE: SERMON ON THE BLOCKS!",
    "PRAYER CIRCLE FOR SUCCESSFUL BLOCK PLACEMENT!",
    "BIBLE STUDY: PARABLE OF THE FALLING BLOCKS!",
    "CHURCH POTLUCK WITH BLOCK-SHAPED FOODS!",
    "YOUTH GROUP BLOCK STACKING MINISTRY!",
    "SALVATION THROUGH BLOCK ACCEPTANCE!",
    
    // Criticism of foreign systems
    "COMMUNIST BLOCKS FALL IN STRAIGHT LINES!",
    "SOCIALIST BLOCK SHARING LEADS TO SHORTAGE!",
    "AUTHORITARIAN BLOCK PLACEMENT LACKS CREATIVITY!",
    "DICTATORSHIPS CONTROL BLOCK ROTATION RIGHTS!",
    "TOTALITARIAN REGIMES FEAR FREE-FALLING BLOCKS!",
    "OPPRESSIVE GOVERNMENTS LIMIT BLOCK CHOICES!",
    "MONARCHY BLOCKS INHERITED, NOT EARNED!",
    "FEUDALISM: PEASANTS STACK BLOCKS FOR LORDS!",
    "FASCIST BLOCKS MARCH IN FORMATION!",
    "TYRANNY CANNOT SUPPRESS BLOCK FREEDOM!",
    
    // Consumerism and brands
    "COCA-COLA: THE REAL THING FOR BLOCK REFRESHMENT!",
    "PEPSI: THE CHOICE OF A NEW BLOCK GENERATION!",
    "NIKE: JUST DO IT... WITH BLOCKS!",
    "APPLE: THINK DIFFERENT ABOUT BLOCK PLACEMENT!",
    "FORD: BUILT TOUGH FOR BLOCK HAULING!",
    "CHEVY: LIKE A ROCK... LIKE A BLOCK!",
    "WALMART: ALWAYS LOW BLOCK PRICES!",
    "TARGET: EXPECT MORE, PAY LESS FOR BLOCKS!",
    "STARBUCKS: GRANDE BLOCK LATTE TO GO!",
    "AMAZON: ONE-CLICK BLOCK ORDERING!",
    
    // Reality TV culture
    "REALITY TV SHOW: KEEPING UP WITH THE BLOCKS!",
    "THE REAL HOUSEWIVES OF BLOCK COUNTY!",
    "DANCING WITH THE BLOCKS!",
    "THE VOICE: BLIND AUDITIONS FOR BLOCK SINGERS!",
    "BIG BROTHER: BLOCK HOUSE SURVEILLANCE!",
    "AMAZING RACE: TEAMS STACK BLOCKS WORLDWIDE!",
    "PROJECT RUNWAY: FASHION FROM BLOCK MATERIALS!",
    "TOP CHEF: COOKING WITH BLOCK INGREDIENTS!",
    "THE BIGGEST LOSER: WEIGHT LOSS THROUGH BLOCK LIFTING!",
    "EXTREME MAKEOVER: BLOCK EDITION!",
    
    // Thanksgiving and holidays
    "THANKSGIVING: GRATEFUL FOR BLOCK ABUNDANCE!",
    "BLACK FRIDAY: DOORBUSTER BLOCK DEALS!",
    "CHRISTMAS: SANTA BRINGS BLOCKS TO GOOD CHILDREN!",
    "FOURTH OF JULY: INDEPENDENCE DAY BLOCK FIREWORKS!",
    "MEMORIAL DAY: HONORING FALLEN BLOCK HEROES!",
    "LABOR DAY: CELEBRATING BLOCK WORKERS!",
    "HALLOWEEN: TRICK OR TREAT FOR BLOCKS!",
    "EASTER: HUNT FOR HIDDEN BLOCK EGGS!",
    "NEW YEAR'S: RESOLUTION TO STACK MORE BLOCKS!",
    "VALENTINE'S DAY: LOVE YOUR BLOCKS!",
    
    // Regional foods
    "SOUTHERN COMFORT FOOD: FRIED BLOCKS AND GRAVY!",
    "NEW ENGLAND CLAM CHOWDER WITH BLOCKS!",
    "CHICAGO DEEP-DISH BLOCK PIZZA!",
    "PHILADELPHIA CHEESESTEAK BLOCK SANDWICH!",
    "KANSAS CITY BBQ BLOCK RIBS!",
    "CALIFORNIA HEALTH FOOD: ORGANIC FREE-RANGE BLOCKS!",
    "TEXAS BBQ: SLOW-SMOKED BLOCKS FOR 12 HOURS!",
    "NEW YORK DELI: PASTRAMI ON RYE WITH BLOCKS!",
    "LOUISIANA JAMBALAYA WITH AUTHENTIC BLOCKS!",
    "MAINE LOBSTER ROLL WITH SIDE OF BLOCKS!",
    
    // Immigration and melting pot
    "AMERICAN DREAM: IMMIGRANTS SEEKING BLOCK FREEDOM!",
    "MELTING POT OF DIVERSE BLOCK CULTURES!",
    "STATUE OF LIBERTY: GIVE US YOUR TIRED BLOCKS!",
    "ELLIS ISLAND: GATEWAY FOR IMMIGRANT BLOCKS!",
    "E PLURIBUS UNUM: OUT OF MANY BLOCKS, ONE!",
    "NATURALIZATION CEREMONY FOR BLOCK CITIZENS!",
    "GREEN CARD LOTTERY FOR LUCKY BLOCKS!",
    "PATHWAY TO CITIZENSHIP THROUGH BLOCK SERVICE!",
    "ENGLISH AS SECOND LANGUAGE FOR FOREIGN BLOCKS!",
    "CULTURAL EXCHANGE PROGRAM FOR INTERNATIONAL BLOCKS!"
};

// Message shown when achieving 4-line clear in American mode
const std::string AMERICAN_EXCELLENCE_MESSAGE = 
    "FREEDOM RINGS WITH EVERY BLOCK! GOD BLESS AMERICA!";
